// The linters' shared core (#199, #278): the YAML subset, the configuration
// and its precedence, rule levels and profiles, NOLINT suppression, output
#include <cstdio>
#include <sstream>
#include <string>
#include "test_harness.hpp"
#include "Lint.hpp"

namespace {

lint::Engine engine() {
    lint::Engine e;
    e.add({"mega65-a", lint::Level::Warning, "all", "rule a", "a explained"});
    e.add({"mega65-b", lint::Level::Info, "cc45", "rule b", ""});
    e.add({"mega65-style-c", lint::Level::Off, "all", "style c", ""});
    return e;
}

void test_yaml() {
    lint::Yaml y;
    std::string err;
    bool ok = lint::parseYaml(
        "# comment\n"
        "profile: cc45\n"
        "c_checks:\n"
        "  enabled: \"mega65-*,-mega65-b\"   # trailing\n"
        "  options:\n"
        "    mega65-stack-frame-limit: 128\n"
        "asm_rules:\n"
        "  - id: mega65-a\n"
        "    severity: error\n"
        "    message: 'it''s quoted'\n"
        "  - id: mega65-style-c\n"
        "    severity: info\n"
        "overrides:\n"
        "  mega65-b: warning\n", y, err);
    CHECK(ok, ("the #278 example parses: " + err).c_str());
    CHECK(ok && y.get("profile") && y.get("profile")->value == "cc45", "a scalar");
    CHECK(ok && y.get("asm_rules") && y.get("asm_rules")->kind == lint::Yaml::List && y.get("asm_rules")->list.size() == 2,
          "a list of maps");
    CHECK(ok && y.get("asm_rules")->list[0].get("message")->value == "it's quoted", "a single-quoted string");
    CHECK(ok && y.get("c_checks")->get("enabled")->value == "mega65-*,-mega65-b", "a double-quoted string, a trailing comment");
    CHECK(!lint::parseYaml("a: 1\n  b: 2\n", y, err) && err.find("line 2") != std::string::npos, "bad indentation, with its line");
    CHECK(!lint::parseYaml("a: [1, 2]\n", y, err), "flow collections are refused");
    CHECK(!lint::parseYaml("a: 1\na: 2\n", y, err) && err.find("duplicate") != std::string::npos, "a duplicate key");
    CHECK(lint::parseYaml("list:\n- x\n- y\n", y, err) && y.get("list")->list.size() == 2, "a list at its key's indentation");
}

void test_config() {
    lint::Yaml y;
    std::string err;
    lint::Config c;
    lint::parseYaml("asm_rules:\n  - id: mega65-a\n    pattern: \"lda $x\"\n", y, err);
    CHECK(!c.apply(y, "f.yml", err) && err.find("f.yml:3") != std::string::npos, ("pattern: refused with its line: " + err).c_str());
    lint::parseYaml("profle: cc45\n", y, err);
    CHECK(!c.apply(y, "f.yml", err) && err.find("unknown key 'profle'") != std::string::npos, "an unknown key");
    lint::parseYaml("profile: cc45\nc_checks:\n  enabled: \"-mega65-a\"\n  options:\n    mega65-x: 7\n", y, err);
    CHECK(c.apply(y, "f.yml", err) && c.profile == "cc45" && c.cChecks.size() == 1 && c.options["mega65-x"] == "7", "applied");
    CHECK(c.applyOption("--disable=mega65-b,mega65-a", err) && c.cliChecks.size() == 2 && c.cliChecks[0] == "-mega65-b",
          "--disable");
    CHECK(!c.applyOption("--profile=nope", err), "a bad --profile");
    CHECK(c.applyOption("--severity=mega65-a=error", err) && c.levels["mega65-a"] == lint::Level::Error, "--severity");
}

void test_levels() {
    lint::Engine e = engine();
    CHECK(e.levelOf("mega65-a") == lint::Level::Warning, "a default");
    CHECK(e.levelOf("mega65-b") == lint::Level::Off, "a cc45 rule is off in the standalone profile");
    e.profile = "cc45";
    CHECK(e.levelOf("mega65-b") == lint::Level::Info, "and on in cc45");
    CHECK(e.levelOf("mega65-style-c") == lint::Level::Off, "off by default");
    e.config.levels["mega65-style-c"] = lint::Level::Info;
    CHECK(e.levelOf("mega65-style-c") == lint::Level::Info, "a severity in the file enables it");
    e.config.cliChecks.push_back("-mega65-style-*");
    CHECK(e.levelOf("mega65-style-c") == lint::Level::Off, "the command line wins over the file");
    e.config.cliChecks.push_back("mega65-style-c");
    CHECK(e.levelOf("mega65-style-c") == lint::Level::Info, "globs apply in order");
    e.lang = "c";
    e.config.cChecks.push_back("-mega65-*");
    CHECK(e.levelOf("mega65-a") == lint::Level::Off, "c_checks.enabled for the C linter");
    e.lang = "asm";
    CHECK(e.levelOf("mega65-a") == lint::Level::Warning, "not for the assembly linter");
    CHECK(e.levelOf("mega65-nope") == lint::Level::Off, "an unknown id");
    CHECK(lint::globMatch("mega65-*", "mega65-dead-store") && !lint::globMatch("mega65-s*x", "mega65-sx-y"), "globs");
}

void test_suppression_and_output() {
    lint::Engine e = engine();
    e.scanSuppressions("a.s45",
        "  lda #1\n"
        "  sta $20 ; NOLINT(mega65-a)\n"
        "  ; NOLINTNEXTLINE\n"
        "  sta $21\n"
        "  sta $22 ; NOLINT(mega65-other)\n"
        "  .ascii \"NOLINT\"\n");
    e.report("mega65-a", "a.s45", 2, 3, "suppressed by id");
    e.report("mega65-a", "a.s45", 4, 3, "suppressed by NOLINTNEXTLINE");
    e.report("mega65-a", "a.s45", 5, 3, "another id: kept");
    e.report("mega65-a", "a.s45", 6, 3, "NOLINT not in a comment: kept");
    e.report("mega65-b", "a.s45", 1, 1, "profile cc45 only: dropped");
    e.suppressFrom("b.c", 10, "mega65-a", false);
    e.suppressFrom("b.c", 20, "mega65-a", true);
    e.report("mega65-a", "b.c", 15, 1, "a pragma range");
    e.report("mega65-a", "b.c", 20, 1, "after the restore: \"kept\"");
    CHECK(e.diagnostics().size() == 3, ("kept: " + std::to_string(e.diagnostics().size())).c_str());
    std::ostringstream t, j, s;
    e.write(t, "text", "al45");
    CHECK(t.str().find("a.s45:5:3: warning: another id: kept [mega65-a]\n") == 0, ("text output, sorted: " + t.str()).c_str());
    e.write(j, "json", "al45");
    CHECK(j.str().find("\"message\": \"after the restore: \\\"kept\\\"\"") != std::string::npos, "JSON, escaped");
    e.write(s, "sarif", "al45");
    CHECK(s.str().find("\"version\": \"2.1.0\"") != std::string::npos && s.str().find("\"ruleId\": \"mega65-a\"") != std::string::npos &&
          s.str().find("\"startLine\": 20") != std::string::npos && s.str().find("\"id\": \"mega65-b\"") == std::string::npos,
          "SARIF: results, only the rules in force");
    e.config.levels["mega65-a"] = lint::Level::Error;
    e.report("mega65-a", "c.s45", 1, 1, "an error");
    CHECK(e.anyError(), "an error makes the run fail");
    e.config.messages["mega65-a"] = "custom";
    e.report("mega65-a", "c.s45", 2, 1, "built in");
    CHECK(e.diagnostics().back().message == "custom", "a configured message");
    std::ostringstream x;
    CHECK(e.explain(x, "mega65-a") && x.str().find("a explained") != std::string::npos && !e.explain(x, "mega65-nope"), "--explain");
}

}  // namespace

int main() {
    test_yaml();
    test_config();
    test_levels();
    test_suppression_and_output();
    printf("\nLint core Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
