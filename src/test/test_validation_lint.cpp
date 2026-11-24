// The linters' shared core (#199, #278): the YAML subset, the configuration
// and its precedence, rule levels and profiles, NOLINT suppression, output
#include <cstdio>
#include <map>
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
    CHECK(!c.apply(y, "f.yml", err) && err.find("f.yml:2") != std::string::npos && err.find("built-in rules'") != std::string::npos,
          ("a pattern under a mega65- id refused with its line: " + err).c_str());
    lint::parseYaml("profle: cc45\n", y, err);
    CHECK(!c.apply(y, "f.yml", err) && err.find("unknown key 'profle'") != std::string::npos, "an unknown key");
    lint::parseYaml("profile: cc45\nc_checks:\n  enabled: \"-mega65-a\"\n  options:\n    mega65-stack-frame-limit: 7\n", y, err);
    CHECK(c.apply(y, "f.yml", err) && c.profile == "cc45" && c.cChecks.size() == 1 &&
          c.options["mega65-stack-frame-limit"] == "7", "applied");
    // #708: what nothing reads is a warning with the file and line, the rest applied
    lint::Config w;
    lint::parseYaml("options:\n  mega65-x: 7\n  mega65-stack-frame-limit: big\n"
                    "overrides:\n  mega65-unreachable: off\n  mega65-nope: error\n", y, err);
    CHECK(w.apply(y, "f.yml", err) && w.warnings.size() == 3 && !w.options.count("mega65-x") &&
          !w.options.count("mega65-stack-frame-limit") && w.levels.count("mega65-unreachable"),
          "unknown option, rule id, a value not a number: warnings");
    CHECK(w.warnings.size() == 3 && w.warnings[0] == "f.yml:2: options: unknown option 'mega65-x' ignored" &&
          w.warnings[1] == "f.yml:3: options: mega65-stack-frame-limit: 'big' is not a number; 256 is used" &&
          w.warnings[2] == "f.yml:6: overrides: unknown rule 'mega65-nope' ignored", "the warnings' text");
    CHECK(lint::knownRules().count("mega65-unreachable") && lint::knownRules().count("mega65-stack-frame-limit") &&
          lint::knownRules().count("mega65-style-whitespace"), "knownRules: al45's, cl45's and the style rules");
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

// user-written rules (#765): the steps, the operand patterns, the configuration
void test_patterns() {
    lint::PatternStep st;
    std::string err;
    CHECK(lint::parsePatternStep("LDA|ld* #{v}", st, err) && st.mnemonics.size() == 2 && st.mnemonics[0] == "lda" &&
          st.mnemonics[1] == "ld*" && !st.anyOperand && st.operand == "#{v}", "a step: mnemonics, an operand pattern");
    CHECK(lint::parsePatternStep("rts", st, err) && st.anyOperand, "a step without an operand: any operand");
    CHECK(!lint::parsePatternStep("lda {x", st, err) && err.find("capture") != std::string::npos, "an unclosed capture");
    CHECK(!lint::parsePatternStep("l$a", st, err), "a bad mnemonic");
    std::map<std::string, std::string> caps;
    CHECK(lint::matchOperand("({p}),y", "($20),Y", caps) && caps["p"] == "$20", "a capture");
    CHECK(lint::matchOperand("{p}", "$20", caps) && !lint::matchOperand("{p}", "$21", caps), "a bound capture: the same text");
    CHECK(lint::matchOperand("*,x", "$1234,x", caps) && lint::matchOperand("*", "", caps) && !lint::matchOperand("#0", "#$00", caps),
          "* any text; literal text as written");
    caps.clear();
    CHECK(!lint::matchOperand("{a}+{a}", "1+2", caps) && caps.empty(), "no match leaves the captures alone");

    lint::Config c;
    lint::Yaml y;
    lint::parseYaml("overrides:\n  my-x: error\nasm_rules:\n  - id: my-x\n    message: \"m {d}\"\n"
                    "    pattern:\n      - \"lda #0\"\n      - \"sta {d}\"\n", y, err);
    CHECK(c.apply(y, "f.yml", err) && c.patterns.size() == 1 && c.patterns[0].steps.size() == 2 && c.warnings.empty() &&
          c.levels["my-x"] == lint::Level::Error, ("a user-written rule; an override before it knows it: " + err).c_str());
    lint::Engine e;
    c.addPatternRules(e);
    e.config = c;
    CHECK(e.find("my-x") && e.levelOf("my-x") == lint::Level::Error, "the rule in the engine, its level from overrides");
    lint::parseYaml("asm_rules:\n  - id: my-y\n    pattern: nop\n", y, err);
    CHECK(!c.apply(y, "f.yml", err) && err.find("f.yml:2") != std::string::npos && err.find("needs a message") != std::string::npos,
          "a rule without a message");
    lint::parseYaml("c_checks:\n  pattern: nop\n", y, err);
    CHECK(!c.apply(y, "f.yml", err) && err.find("al45's only") != std::string::npos, "c_checks.pattern refused");
}

}  // namespace

int main() {
    test_yaml();
    test_config();
    test_levels();
    test_suppression_and_output();
    test_patterns();
    printf("\nLint core Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
