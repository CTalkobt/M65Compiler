#ifndef BASICSTANDARDLIBRARY_HPP
#define BASICSTANDARDLIBRARY_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

// Standard library function definition
struct StdlibFunction {
    std::string name;
    std::string category;      // "STRING", "MATH", "IO", "MEMORY", "SYSTEM"
    std::string signature;     // e.g., "INT(INT, INT)"
    std::string description;
    std::vector<std::string> dependencies;  // Other functions this depends on
    std::string assemblyCode;  // Implementation in assembly
    int byteSize;              // Size of compiled function
    bool required;             // Always include
};

// Standard library category
struct StdlibCategory {
    std::string name;
    std::string description;
    std::vector<StdlibFunction> functions;
    int totalSize;
};

// Standard library injector
class BasicStandardLibraryInjector {
public:
    BasicStandardLibraryInjector();

    // Initialize standard library database
    void initializeLibrary();

    // Scan source code for function calls
    std::set<std::string> scanFunctionCalls(const std::string& sourceCode);

    // Get required functions (with dependency resolution)
    std::vector<StdlibFunction> getRequiredFunctions(const std::set<std::string>& usedFunctions);

    // Inject library code into source
    std::string injectLibraryCode(
        const std::string& sourceCode,
        const std::vector<StdlibFunction>& functions
    );

    // Generate library initialization code
    std::string generateLibraryInit();

    // Get function by name
    const StdlibFunction* getFunction(const std::string& name) const;

    // Get all functions in category
    std::vector<StdlibFunction> getFunctionsInCategory(const std::string& category) const;

    // Calculate total library size
    int calculateLibrarySize(const std::vector<StdlibFunction>& functions) const;

    // Get library statistics
    struct LibraryStats {
        int totalFunctions;
        int totalSize;
        int categoryCount;
        std::map<std::string, int> sizeByCategory;
    };

    LibraryStats getStatistics() const;

private:
    std::map<std::string, StdlibCategory> library;
    std::map<std::string, StdlibFunction> functionLookup;

    // Build standard library database
    void buildStringFunctions();
    void buildMathFunctions();
    void buildIOFunctions();
    void buildMemoryFunctions();
    void buildSystemFunctions();

    // Dependency resolution
    void resolveDependencies(
        const std::set<std::string>& functions,
        std::set<std::string>& resolved
    );
};

// Library configuration
struct LibraryConfig {
    bool autoInject = true;           // Automatically inject used functions
    bool minimizeSize = true;         // Only include needed functions
    bool generateInit = true;         // Generate initialization code
    std::set<std::string> excludeFunctions;  // Functions to exclude
    std::set<std::string> includeFunctions;  // Force include functions
    int maxLibrarySize = 65536;       // Max library size in bytes
};

// Smart library manager
class SmartLibraryManager {
public:
    SmartLibraryManager(const LibraryConfig& config = LibraryConfig());

    // Analyze and inject library into program
    bool analyzeProgramAndInjectLibrary(
        std::string& sourceCode,
        std::string& errorMessage
    );

    // Get injection report
    struct InjectionReport {
        std::vector<std::string> injectedFunctions;
        int totalBytesAdded;
        std::vector<std::string> warnings;
        bool success;
    };

    InjectionReport getLastReport() const { return lastReport; }

    // Set configuration
    void setConfig(const LibraryConfig& config) { this->config = config; }

private:
    BasicStandardLibraryInjector injector;
    LibraryConfig config;
    InjectionReport lastReport;
};

// Function call analyzer
class FunctionCallAnalyzer {
public:
    // Analyze source code for function calls
    static std::set<std::string> analyzeCalls(const std::string& sourceCode);

    // Find function call positions
    struct CallInfo {
        std::string functionName;
        int lineNumber;
        int columnNumber;
        std::string context;
    };

    static std::vector<CallInfo> findFunctionCalls(const std::string& sourceCode);

    // Check if function is built-in
    static bool isBuiltinFunction(const std::string& name);

private:
    // Standard built-in functions
    static const std::set<std::string> BUILTIN_FUNCTIONS;
};

#endif  // BASICSTANDARDLIBRARY_HPP
