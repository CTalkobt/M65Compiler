// Dbg45Dap.cpp — dbg45 --dap: the Debug Adapter Protocol on stdin/stdout,
// for an editor's debugger (VS Code: tools/ide/vscode-extension, the "cc45"
// debug type). Requests: initialize, launch, setBreakpoints,
// setFunctionBreakpoints, setExceptionBreakpoints, configurationDone,
// threads, stackTrace, scopes, variables, evaluate, continue, next,
// stepIn, stepOut, pause, disconnect, terminate. One thread: the MEGA65.
//
// A reader thread queues the requests; the program runs on the main one,
// which looks for a pause or disconnect while it waits for a stop.
#include "Dbg45.hpp"
#include "MiniJson.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace {

struct Dap {
    Dbg45& d;
    int seq = 1;
    std::mutex outLock;
    std::mutex qLock;
    std::condition_variable qCv;
    std::deque<Json> queue;
    std::atomic<bool> inputClosed{false}, pauseAsked{false}, quitAsked{false};
    bool stopOnEntry = false;
    std::string program;
    // variables references: 1000 + frame * 4 + 0 locals / 1 arguments /
    // 2 globals; an aggregate's children from 100000 on
    std::map<int, Dbg45Value> handles;
    int nextHandle = 100000;
    std::map<std::string, std::vector<int>> fileBps;   // an editor's source path -> breakpoint ids
    std::vector<int> funcBps;

    explicit Dap(Dbg45& d_) : d(d_) {}

    void send(Json msg) {
        std::lock_guard<std::mutex> g(outLock);
        msg.set("seq", seq++);
        std::string body = msg.dump();
        std::string out = "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
        fwrite(out.data(), 1, out.size(), stdout);
        fflush(stdout);
    }
    void respond(const Json& req, bool ok, Json body = Json::object(), const std::string& message = "") {
        Json r = Json::object();
        r.set("type", "response").set("request_seq", req.numAt("seq")).set("success", ok).set("command", req.strAt("command"));
        if (!message.empty()) r.set("message", message);
        r.set("body", std::move(body));
        send(r);
    }
    void event(const std::string& name, Json body = Json::object()) {
        Json e = Json::object();
        e.set("type", "event").set("event", name).set("body", std::move(body));
        send(e);
    }
    void output(const std::string& text, const char* category = "console") {
        Json b = Json::object();
        b.set("category", category).set("output", text);
        event("output", b);
    }

    // stdin: Content-Length headers, then the JSON
    void reader() {
        for (;;) {
            size_t len = 0;
            for (;;) {
                std::string line;
                int c;
                while ((c = fgetc(stdin)) != EOF && c != '\n') line += (char)c;
                if (c == EOF) { inputClosed = true; qCv.notify_all(); return; }
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.empty()) break;
                if (line.rfind("Content-Length:", 0) == 0) len = (size_t)std::stoul(line.substr(15));
            }
            std::string body(len, '\0');
            if (len && fread(&body[0], 1, len, stdin) != len) { inputClosed = true; qCv.notify_all(); return; }
            Json msg;
            if (!Json::parse(body, msg)) continue;
            std::string cmd = msg.strAt("command");
            if (cmd == "pause") pauseAsked = true;
            if (cmd == "disconnect" || cmd == "terminate") quitAsked = true;
            std::lock_guard<std::mutex> g(qLock);
            queue.push_back(std::move(msg));
            qCv.notify_all();
        }
    }

    bool next(Json& msg) {
        std::unique_lock<std::mutex> g(qLock);
        qCv.wait(g, [&] { return !queue.empty() || inputClosed; });
        if (queue.empty()) return false;
        msg = std::move(queue.front());
        queue.pop_front();
        return true;
    }

    // after a run / step: the stop as an event (or the end)
    void stopped(Dbg45::Stop s, const std::string& err, const char* reason) {
        handles.clear();
        nextHandle = 100000;
        if (s == Dbg45::EXITED) {
            Json b = Json::object();
            b.set("exitCode", d.exitStatus());
            output("[program exited with status " + std::to_string(d.exitStatus()) + "]\n");
            event("exited", b);
            event("terminated");
            d.kill();
            return;
        }
        if (s == Dbg45::ERROR) {
            output("dbg45: " + err + "\n", "stderr");
            event("terminated");
            d.kill();
            return;
        }
        Json b = Json::object();
        const char* why = s == Dbg45::BREAKPOINT ? "breakpoint" : s == Dbg45::PAUSED || s == Dbg45::TIMEOUT ? "pause" : reason;
        b.set("reason", why).set("threadId", 1).set("allThreadsStopped", true);
        if (s == Dbg45::BREAKPOINT) {
            Json ids = Json::array();
            ids.push(Json::number(d.lastBreakpoint()));
            b.set("hitBreakpointIds", ids);
        }
        event("stopped", b);
    }

    // a source file's path for the editor: as recorded when absolute, else
    // from the current directory or the program's
    std::string sourcePath(int file) {
        std::string f = d.dbg.files[file];
        if (!f.empty() && f[0] == '/') return f;
        char cwd[4096];
        if (access(f.c_str(), R_OK) == 0 && getcwd(cwd, sizeof cwd)) return std::string(cwd) + "/" + f;
        size_t s = program.find_last_of('/');
        std::string p = (s == std::string::npos ? "" : program.substr(0, s + 1)) + f;
        if (access(p.c_str(), R_OK) == 0) {
            if (!p.empty() && p[0] != '/' && getcwd(cwd, sizeof cwd)) p = std::string(cwd) + "/" + p;
            return p;
        }
        return f;
    }

    Json variable(const std::string& name, const Dbg45Value& v) {
        Json j = Json::object();
        j.set("name", name).set("value", d.format(v)).set("type", v.type);
        bool aggregate = !v.type.empty() && (v.type[0] == '[' || v.type[0] == '&' || v.type.rfind("struct:", 0) == 0 ||
                                             v.type.rfind("union:", 0) == 0);
        if (aggregate) {
            handles[nextHandle] = v;
            j.set("variablesReference", nextHandle++);
        } else j.set("variablesReference", 0);
        return j;
    }

    void handle(const Json& req) {
        std::string cmd = req.strAt("command");
        const Json* argsP = req.at("arguments");
        Json args = argsP ? *argsP : Json::object();
        std::string err;
        if (cmd == "initialize") {
            Json caps = Json::object();
            caps.set("supportsConfigurationDoneRequest", true).set("supportsFunctionBreakpoints", true)
                .set("supportsEvaluateForHovers", true).set("supportsTerminateRequest", true);
            respond(req, true, caps);
            event("initialized");
        } else if (cmd == "launch") {
            program = args.strAt("program");
            d.opt.headless = args.boolAt("headless", false);
            d.opt.fast = args.boolAt("fast", false);
            if (!args.strAt("xemu").empty()) d.opt.xemu = args.strAt("xemu");
            if (const Json* xa = args.at("xemuArgs")) for (const auto& a : xa->arr) d.opt.xemuArgs.push_back(a.str);
            stopOnEntry = args.boolAt("stopOnEntry", false);
            if (!d.open(program, err)) { respond(req, false, Json::object(), err); return; }
            respond(req, true);
        } else if (cmd == "setBreakpoints") {
            std::string path;
            if (const Json* src = args.at("source")) path = src->strAt("path");
            for (int id : fileBps[path]) d.deleteBreakpoint(id);
            fileBps[path].clear();
            Json list = Json::array();
            if (const Json* bps = args.at("breakpoints"))
                for (const auto& bp : bps->arr) {
                    int line = (int)bp.numAt("line");
                    Json r = Json::object();
                    int id = d.addBreakpoint(path + ":" + std::to_string(line), err);
                    if (id) {
                        fileBps[path].push_back(id);
                        const std::string& where = d.breakpoints[id].where;
                        int at = std::stoi(where.substr(where.rfind(':') + 1));
                        r.set("id", id).set("verified", true).set("line", at);
                    } else r.set("verified", false).set("message", err).set("line", line);
                    list.push(r);
                }
            Json b = Json::object();
            b.set("breakpoints", list);
            respond(req, true, b);
        } else if (cmd == "setFunctionBreakpoints") {
            for (int id : funcBps) d.deleteBreakpoint(id);
            funcBps.clear();
            Json list = Json::array();
            if (const Json* bps = args.at("breakpoints"))
                for (const auto& bp : bps->arr) {
                    Json r = Json::object();
                    int id = d.addBreakpoint(bp.strAt("name"), err);
                    if (id) { funcBps.push_back(id); r.set("id", id).set("verified", true); }
                    else r.set("verified", false).set("message", err);
                    list.push(r);
                }
            Json b = Json::object();
            b.set("breakpoints", list);
            respond(req, true, b);
        } else if (cmd == "setExceptionBreakpoints") {
            Json b = Json::object();
            b.set("breakpoints", Json::array());
            respond(req, true, b);
        } else if (cmd == "configurationDone") {
            respond(req, true);
            pauseAsked = false;
            Dbg45::Stop s = d.run(stopOnEntry, err);
            stopped(s, err, "entry");
        } else if (cmd == "threads") {
            Json t = Json::object();
            t.set("id", 1).set("name", "MEGA65");
            Json list = Json::array();
            list.push(t);
            Json b = Json::object();
            b.set("threads", list);
            respond(req, true, b);
        } else if (cmd == "stackTrace") {
            Json frames = Json::array();
            auto bt = d.backtrace();
            for (size_t i = 0; i < bt.size(); i++) {
                Json f = Json::object();
                f.set("id", (int)i).set("name", bt[i].function ? d.dbg.cName(bt[i].function->name) : "??");
                if (bt[i].line) {
                    Json src = Json::object();
                    std::string p = sourcePath(bt[i].line->file);
                    src.set("name", p.substr(p.find_last_of('/') == std::string::npos ? 0 : p.find_last_of('/') + 1)).set("path", p);
                    f.set("source", src).set("line", bt[i].line->line).set("column", 1);
                } else f.set("line", 0).set("column", 0);
                char pc[8];
                snprintf(pc, sizeof pc, "$%04x", bt[i].pc);
                f.set("instructionPointerReference", pc);
                frames.push(f);
            }
            Json b = Json::object();
            b.set("stackFrames", frames).set("totalFrames", (int)bt.size());
            respond(req, true, b);
        } else if (cmd == "scopes") {
            int fr = (int)args.numAt("frameId");
            Json list = Json::array();
            const char* names[3] = {"Locals", "Arguments", "Globals"};
            for (int k = 0; k < 3; k++) {
                Json sc = Json::object();
                sc.set("name", names[k]).set("variablesReference", 1000 + fr * 4 + k).set("expensive", k == 2);
                list.push(sc);
            }
            Json b = Json::object();
            b.set("scopes", list);
            respond(req, true, b);
        } else if (cmd == "variables") {
            int ref = (int)args.numAt("variablesReference");
            Json list = Json::array();
            if (ref >= 100000) {
                auto it = handles.find(ref);
                if (it != handles.end()) {
                    Dbg45Value v = it->second;
                    for (const auto& [n, c] : d.children(v)) list.push(variable(n, c));
                }
            } else if (ref >= 1000) {
                size_t fr = (size_t)((ref - 1000) / 4);
                int kind = (ref - 1000) % 4;
                auto bt = d.backtrace();
                std::vector<const DbgVar*> vars;
                if (kind == 2) {
                    int obj = fr < bt.size() && bt[fr].function ? bt[fr].function->object : -1;
                    for (const auto& v : d.dbg.variables)
                        if (v.scope.empty() && v.object == obj) vars.push_back(&v);
                } else if (fr < bt.size() && bt[fr].function) {
                    for (const DbgVar* v : d.dbg.variablesOf(bt[fr].function->name))
                        if ((v->kind == "param") == (kind == 1)) vars.push_back(v);
                }
                for (const DbgVar* v : vars) {
                    Dbg45Value val;
                    std::string e;
                    if (d.evaluateVar(*v, fr, val, e)) list.push(variable(v->name, val));
                    else {
                        Json j = Json::object();
                        j.set("name", v->name).set("value", e).set("type", v->type).set("variablesReference", 0);
                        list.push(j);
                    }
                }
            }
            Json b = Json::object();
            b.set("variables", list);
            respond(req, true, b);
        } else if (cmd == "evaluate") {
            Dbg45Value v;
            size_t fr = (size_t)args.numAt("frameId");
            if (!d.evaluate(args.strAt("expression"), fr, v, err)) { respond(req, false, Json::object(), err); return; }
            Json j = variable("", v);
            Json b = Json::object();
            b.set("result", j.strAt("value")).set("type", v.type).set("variablesReference", j.numAt("variablesReference"));
            respond(req, true, b);
        } else if (cmd == "continue" || cmd == "next" || cmd == "stepIn" || cmd == "stepOut") {
            Json b = Json::object();
            b.set("allThreadsContinued", true);
            respond(req, true, b);
            pauseAsked = false;
            Dbg45::Stop s = cmd == "continue" ? d.cont(err) : cmd == "next" ? d.stepLine(true, err)
                          : cmd == "stepIn" ? d.stepLine(false, err) : d.finish(err);
            stopped(s, err, "step");
        } else if (cmd == "pause") {
            // (a pause that came while the program ran stopped that run; here
            // it is already stopped, or this pauses it)
            pauseAsked = false;
            respond(req, true);
            if (d.started() && !d.exited()) {
                Dbg45::Stop s = d.pause(err);
                if (s != Dbg45::ERROR) stopped(Dbg45::PAUSED, err, "pause");
            }
        } else if (cmd == "disconnect" || cmd == "terminate") {
            d.kill();
            respond(req, true);
            if (cmd == "terminate") event("terminated");
        } else {
            respond(req, false, Json::object(), "dbg45 does not handle '" + cmd + "'");
        }
    }
};

}  // namespace

int dbg45Dap(Dbg45& d) {
    Dap dap(d);
    // a pause or a disconnect while the program runs
    d.opt.interrupt = [&] { return dap.pauseAsked.load() || dap.quitAsked.load() || dap.inputClosed.load(); };
    std::thread reader([&] { dap.reader(); });
    reader.detach();
    Json msg;
    while (dap.next(msg)) {
        dap.handle(msg);
        std::string cmd = msg.strAt("command");
        if (cmd == "disconnect" || cmd == "terminate") break;
    }
    d.kill();
    fflush(stdout);
    _exit(0);   // (the reader thread may sit in a read)
}
