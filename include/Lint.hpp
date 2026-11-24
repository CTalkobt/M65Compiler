#pragma once
// The linters' shared core (#199, #278; doc/architecture/lint.md): rules by
// stable mega65-* ids, the .cc45-lint.yml configuration, inline
// suppression, text / JSON / SARIF output. Used by al45 (ca45's lint mode)
// and cl45 (cc45's).

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace lint {

enum class Level { Off, Info, Warning, Error };
const char* levelName(Level l);
bool parseLevel(const std::string& s, Level& out);

struct Rule {
    std::string id;           // mega65-...
    Level level = Level::Warning;   // the default
    std::string profile;      // "all", "cc45" or "standalone"
    std::string summary;      // one line (--list-rules, SARIF)
    std::string explanation;  // --explain
};

struct Diagnostic {
    std::string file;
    int line = 0, col = 0;
    Level level = Level::Warning;
    std::string ruleId, message;
};

// A YAML subset: block maps and lists by indentation, scalars (plain,
// "double" or 'single' quoted), # comments. Enough for .cc45-lint.yml.
struct Yaml {
    enum Kind { Scalar, Map, List } kind = Scalar;
    std::string value;                                  // Scalar
    std::vector<std::pair<std::string, Yaml>> map;      // Map (in order)
    std::vector<Yaml> list;                             // List
    int line = 0;
    const Yaml* get(const std::string& key) const;
};
// false and err = "line N: ..." on a syntax error
bool parseYaml(const std::string& text, Yaml& out, std::string& err);

class Engine;

// A user-written rule (#765): asm_rules entry with a pattern, a sequence of
// instructions al45 looks for (AsmLint.cpp runPatterns; doc/bin/al45.md)
struct PatternStep {
    std::vector<std::string> mnemonics;   // lower case globs (ld*; *: any instruction), one to match
    bool anyOperand = true;               // no operand pattern given
    std::string operand;                  // the operand pattern (spaces removed, lower case)
    std::string text;                     // as written (for --explain)
};
struct PatternRule {
    std::string id, message, file;
    int line = 0;
    Level level = Level::Warning;
    std::vector<PatternStep> steps;
};
// one step "mnemonic[|mnemonic...] [operand pattern]"; false and err on a malformed one
bool parsePatternStep(const std::string& text, PatternStep& out, std::string& err);
// an operand pattern over an operand's text (both spaces removed, lower case):
// * any run, {name} a capture (a non-empty run, the same text where bound again)
bool matchOperand(const std::string& pat, const std::string& text, std::map<std::string, std::string>& caps);

// The configuration in force: what the files, the command line and the
// defaults say, in that order of precedence (inline suppression is the
// Engine's).
struct Config {
    std::string profile;                       // "" = the tool's default
    // clang-tidy style globs, in order ("-x" removes): c_checks.enabled (C),
    // the command line's --checks / --disable (either tool)
    std::vector<std::string> cChecks, cliChecks;
    std::map<std::string, Level> levels;       // severity per id
    std::map<std::string, std::string> messages;   // message per id
    std::map<std::string, std::string> options;    // mega65-...: value
    std::vector<std::string> sources;          // the files read (for -v)
    std::vector<PatternRule> patterns;         // user-written rules (al45's; a later file's id replaces)
    // what was ignored, "file:line: ..." (#708): an unknown option or rule id,
    // a glob matching no rule, an option value that is not a number
    std::vector<std::string> warnings;
    // a file's settings over these; false (and err) on an error in it
    bool apply(const Yaml& y, const std::string& file, std::string& err);
    bool loadFile(const std::string& path, std::string& err);
    // ~/.config/m65/cc45-lint.yml, then the first .cc45-lint.yml from dir upward
    bool loadDefaults(const std::string& dir, std::string& err);
    // --checks=LIST / --disable=ID[,ID] / --profile=P / --severity=ID=LEVEL
    bool applyOption(const std::string& arg, std::string& err);
    // the user-written rules as the engine's rules (al45)
    void addPatternRules(Engine& e) const;
    // the warnings on os as "<tool>: warning: ..." (each once per Config)
    void printWarnings(std::ostream& os, const std::string& tool);
private:
    size_t warningsPrinted_ = 0;
    std::set<std::string> userIds_;            // the ids of patterns (known to the warnings)
    bool known(const std::string& id) const;
    bool patternRule(const Yaml& r, const std::string& file, std::set<std::string>& idsHere, std::string& err);
    void option(const std::string& where, const std::string& key, const Yaml& v);
    void ruleId(const std::string& where, const std::string& id);
    void ruleGlob(const std::string& where, const std::string& glob);
};

// Every rule id of al45 and cl45 (one configuration serves both) and the
// options the rules read (LintRules.cpp)
const std::set<std::string>& knownRules();
struct OptionInfo {
    const char* id;
    const char* dflt;   // (what is used when it is not set)
    bool number;        // a value that must be a whole number (a warning, and the default, when not)
};
const std::vector<OptionInfo>& knownOptions();

class Engine {
public:
    void add(const Rule& r);
    const std::vector<Rule>& rules() const { return rules_; }
    const Rule* find(const std::string& id) const;
    Config config;
    std::string profile = "standalone";        // the profile in force (set by the tool)
    std::string lang = "asm";                  // "asm" (al45) or "c" (cl45)

    // a rule's level in force (Off when not enabled or not of the profile)
    Level levelOf(const std::string& id) const;
    std::string option(const std::string& id, const std::string& dflt) const;

    // NOLINT / NOLINTNEXTLINE comments of a file's lines (prefix: ";" or "//")
    void scanSuppressions(const std::string& file, const std::string& text);
    // C: #pragma cc45 lint(ignore, id) from a line on (restore: to it)
    void suppressFrom(const std::string& file, int line, const std::string& id, bool restore);

    // a finding: kept when the rule is on and not suppressed there
    void report(const std::string& id, const std::string& file, int line, int col, const std::string& message);
    const std::vector<Diagnostic>& diagnostics() const { return diags_; }
    bool anyError() const;

    void write(std::ostream& os, const std::string& format, const std::string& tool) const;
    void listRules(std::ostream& os) const;
    bool explain(std::ostream& os, const std::string& id) const;

private:
    bool suppressed(const std::string& id, const std::string& file, int line) const;
    std::vector<Rule> rules_;
    std::vector<Diagnostic> diags_;
    // file -> line -> ids ("*": all)
    std::map<std::string, std::map<int, std::set<std::string>>> nolint_;
    struct Range { int from, to; std::string id; };
    std::map<std::string, std::vector<Range>> ranges_;
};

// glob with * (and nothing else)
bool globMatch(const std::string& pat, const std::string& s);

}  // namespace lint
