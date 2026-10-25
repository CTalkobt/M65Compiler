#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iosfwd>

class Preprocessor {
public:
    Preprocessor(bool isCompiler = false);
    
    // Process the source code.
    // symbols: initial set of defined symbols (e.g. from -D flags)
    // includePaths: directories to search for #include files
    // currentFile: path to the file being processed (used for relative includes)
    std::string process(const std::string& source, 
                        const std::map<std::string, std::string>& initialSymbols,
                        const std::vector<std::string>& includePaths,
                        const std::string& currentFile);

private:
    bool isCompiler;
    
    struct Macro {
        bool isFunctionLike = false;
        bool isVariadic = false;
        std::vector<std::string> params;
        std::string body;
        // where it was defined: a file and line, "<command line>" (-D) or
        // "" (predefined)
        std::string defFile;
        int defLine = 0;
    };
    // #define name: a different redefinition is diagnosed (C11 6.10.3p2)
    void defineMacro(const std::string& name, Macro m, const std::string& file, int line);
    static bool sameDefinition(const Macro& a, const Macro& b);
    std::map<std::string, Macro> macros;
    // #pragma push_macro("X") / pop_macro("X"): saved definitions (absent = not defined)
    std::map<std::string, std::vector<std::pair<bool, Macro>>> pushedMacros_;
    
    std::vector<std::string> includePaths;
    std::set<std::string> includedFiles; // To prevent infinite recursion
    std::set<std::string> onceFiles;     // To support #includeonce
    std::string preprocDate;
    std::string preprocTime;
    
    struct State {
        bool active = true;
        bool hasSeenTrueBranch = false;
        bool inElse = false;
    };
    std::vector<State> stateStack;

    std::vector<std::string> pendingPragmas_; // collected by expandMacros from _Pragma()
    int includeDepth_ = 0;          // current #include nesting level
    std::string currentFile_;       // current file being processed (for __has_include path resolution)

    std::string processInternal(const std::string& source, const std::string& currentFile, int depth);
    void handlePragma(std::istream& ss, std::ostream& output, const std::string& currentFile);
    bool isConditionTrue();
    long evaluateExpression(const std::string& expr);
    std::string expandMacros(const std::string& line);
    // Macro expansion on tokens (C11 6.10.3): each token's hide set holds
    // the macros it came from, which do not expand it again; arguments are
    // expanded on their own before substitution (not next to # or ##)
    struct PPTok {
        std::string ws;        // the white space before it
        std::string text;
        char kind = 'p';       // 'i' identifier, 'n' pp-number, 's' string/char, 'p' punctuator, 'm' placemarker
        std::set<std::string> hide;
        bool expanded = false; // came from a replacement (spacing kept apart on output)
    };
    static std::vector<PPTok> lexPP(const std::string& text);
    std::vector<PPTok> expandPP(std::vector<PPTok> in, int depth = 0);
    std::vector<PPTok> substPP(const std::vector<PPTok>& body, const Macro& m,
                               const std::vector<std::vector<PPTok>>& args, const std::vector<PPTok>& va,
                               bool haveVa, const std::set<std::string>& hs, int depth);
    static std::string renderPP(const std::vector<PPTok>& toks);
    // __LINE__ / __FILE__ for the line being expanded (0 / "": left to the caller)
    int expandLine_ = 0;
    std::string expandFile_;
    // a call of a function-like macro whose ')' is not on this line
    bool hasOpenMacroCall(const std::string& line) const;
    std::string stripComments(const std::string& line, bool& inBlockComment);
    std::string getDirectory(const std::string& filePath);
    std::string findIncludeFile(const std::string& fileName, const std::string& currentDir);
    std::string trim(const std::string& s);
};
