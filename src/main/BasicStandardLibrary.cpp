#include "BasicStandardLibrary.hpp"
#include <regex>
#include <algorithm>
#include <sstream>

// ==================== FunctionCallAnalyzer ====================

const std::set<std::string> FunctionCallAnalyzer::BUILTIN_FUNCTIONS = {
    "PRINT", "INPUT", "GOTO", "GOSUB", "RETURN", "IF", "THEN", "ELSE",
    "FOR", "NEXT", "WHILE", "WEND", "DO", "LOOP", "EXIT", "CONTINUE",
    "DIM", "REDIM", "LEN", "MID", "LEFT", "RIGHT", "INSTR", "STR",
    "VAL", "ABS", "INT", "SIN", "COS", "TAN", "SQR", "EXP", "LOG"
};

std::set<std::string> FunctionCallAnalyzer::analyzeCalls(const std::string& sourceCode) {
    std::set<std::string> functions;
    std::istringstream iss(sourceCode);
    std::string line;

    // Pattern to match function calls: WORD(
    std::regex funcPattern(R"(\b([A-Za-z_]\w*)\s*\()");

    while (std::getline(iss, line)) {
        std::sregex_iterator it(line.begin(), line.end(), funcPattern);
        std::sregex_iterator end;

        for (; it != end; ++it) {
            std::string funcName = it->str(1);
            if (!isBuiltinFunction(funcName)) {
                functions.insert(funcName);
            }
        }
    }

    return functions;
}

std::vector<FunctionCallAnalyzer::CallInfo> FunctionCallAnalyzer::findFunctionCalls(
    const std::string& sourceCode
) {
    std::vector<CallInfo> calls;
    std::istringstream iss(sourceCode);
    std::string line;
    int lineNum = 1;

    std::regex funcPattern(R"(\b([A-Za-z_]\w*)\s*\()");

    while (std::getline(iss, line)) {
        std::sregex_iterator it(line.begin(), line.end(), funcPattern);
        std::sregex_iterator end;

        for (; it != end; ++it) {
            CallInfo call;
            call.functionName = it->str(1);
            call.lineNumber = lineNum;
            call.columnNumber = it->position() + 1;
            call.context = line;

            calls.push_back(call);
        }

        lineNum++;
    }

    return calls;
}

bool FunctionCallAnalyzer::isBuiltinFunction(const std::string& name) {
    return BUILTIN_FUNCTIONS.find(name) != BUILTIN_FUNCTIONS.end();
}

// ==================== BasicStandardLibraryInjector ====================

BasicStandardLibraryInjector::BasicStandardLibraryInjector() {
    initializeLibrary();
}

void BasicStandardLibraryInjector::initializeLibrary() {
    buildStringFunctions();
    buildMathFunctions();
    buildIOFunctions();
    buildMemoryFunctions();
    buildSystemFunctions();
}

void BasicStandardLibraryInjector::buildStringFunctions() {
    StdlibCategory category;
    category.name = "STRING";
    category.description = "String manipulation functions";
    category.totalSize = 0;

    // STRLEN function
    StdlibFunction strlen_func;
    strlen_func.name = "STRLEN";
    strlen_func.category = "STRING";
    strlen_func.signature = "INT(STRING)";
    strlen_func.description = "Return length of string";
    strlen_func.byteSize = 12;
    strlen_func.required = false;
    category.functions.push_back(strlen_func);
    functionLookup[strlen_func.name] = strlen_func;

    // SUBSTR function
    StdlibFunction substr_func;
    substr_func.name = "SUBSTR";
    substr_func.category = "STRING";
    substr_func.signature = "STRING(STRING, INT, INT)";
    substr_func.description = "Extract substring from position with length";
    substr_func.byteSize = 24;
    substr_func.required = false;
    category.functions.push_back(substr_func);
    functionLookup[substr_func.name] = substr_func;

    // STRCAT function
    StdlibFunction strcat_func;
    strcat_func.name = "STRCAT";
    strcat_func.category = "STRING";
    strcat_func.signature = "STRING(STRING, STRING)";
    strcat_func.description = "Concatenate two strings";
    strcat_func.byteSize = 18;
    strcat_func.required = false;
    category.functions.push_back(strcat_func);
    functionLookup[strcat_func.name] = strcat_func;

    library["STRING"] = category;
}

void BasicStandardLibraryInjector::buildMathFunctions() {
    StdlibCategory category;
    category.name = "MATH";
    category.description = "Mathematical functions";
    category.totalSize = 0;

    // MIN function
    StdlibFunction min_func;
    min_func.name = "MIN";
    min_func.category = "MATH";
    min_func.signature = "INT(INT, INT)";
    min_func.description = "Return minimum of two integers";
    min_func.byteSize = 8;
    min_func.required = false;
    category.functions.push_back(min_func);
    functionLookup[min_func.name] = min_func;

    // MAX function
    StdlibFunction max_func;
    max_func.name = "MAX";
    max_func.category = "MATH";
    max_func.signature = "INT(INT, INT)";
    max_func.description = "Return maximum of two integers";
    max_func.byteSize = 8;
    max_func.required = false;
    category.functions.push_back(max_func);
    functionLookup[max_func.name] = max_func;

    // CLAMP function
    StdlibFunction clamp_func;
    clamp_func.name = "CLAMP";
    clamp_func.category = "MATH";
    clamp_func.signature = "INT(INT, INT, INT)";
    clamp_func.description = "Clamp value between min and max";
    clamp_func.byteSize = 16;
    clamp_func.dependencies = {"MIN", "MAX"};
    clamp_func.required = false;
    category.functions.push_back(clamp_func);
    functionLookup[clamp_func.name] = clamp_func;

    library["MATH"] = category;
}

void BasicStandardLibraryInjector::buildIOFunctions() {
    StdlibCategory category;
    category.name = "IO";
    category.description = "Input/Output functions";
    category.totalSize = 0;

    // READFILE function
    StdlibFunction readfile_func;
    readfile_func.name = "READFILE";
    readfile_func.category = "IO";
    readfile_func.signature = "STRING(STRING)";
    readfile_func.description = "Read entire file into string";
    readfile_func.byteSize = 32;
    readfile_func.required = false;
    category.functions.push_back(readfile_func);
    functionLookup[readfile_func.name] = readfile_func;

    // WRITEFILE function
    StdlibFunction writefile_func;
    writefile_func.name = "WRITEFILE";
    writefile_func.category = "IO";
    writefile_func.signature = "INT(STRING, STRING)";
    writefile_func.description = "Write string to file";
    writefile_func.byteSize = 28;
    writefile_func.required = false;
    category.functions.push_back(writefile_func);
    functionLookup[writefile_func.name] = writefile_func;

    library["IO"] = category;
}

void BasicStandardLibraryInjector::buildMemoryFunctions() {
    StdlibCategory category;
    category.name = "MEMORY";
    category.description = "Memory management functions";
    category.totalSize = 0;

    // MEMCOPY function
    StdlibFunction memcopy_func;
    memcopy_func.name = "MEMCOPY";
    memcopy_func.category = "MEMORY";
    memcopy_func.signature = "INT(INT, INT, INT)";
    memcopy_func.description = "Copy memory block (dest, src, size)";
    memcopy_func.byteSize = 20;
    memcopy_func.required = false;
    category.functions.push_back(memcopy_func);
    functionLookup[memcopy_func.name] = memcopy_func;

    // MEMFILL function
    StdlibFunction memfill_func;
    memfill_func.name = "MEMFILL";
    memfill_func.category = "MEMORY";
    memfill_func.signature = "INT(INT, INT, INT)";
    memfill_func.description = "Fill memory block (dest, size, byte)";
    memfill_func.byteSize = 18;
    memfill_func.required = false;
    category.functions.push_back(memfill_func);
    functionLookup[memfill_func.name] = memfill_func;

    library["MEMORY"] = category;
}

void BasicStandardLibraryInjector::buildSystemFunctions() {
    StdlibCategory category;
    category.name = "SYSTEM";
    category.description = "System and utility functions";
    category.totalSize = 0;

    // DELAY function
    StdlibFunction delay_func;
    delay_func.name = "DELAY";
    delay_func.category = "SYSTEM";
    delay_func.signature = "INT(INT)";
    delay_func.description = "Delay for N milliseconds";
    delay_func.byteSize = 10;
    delay_func.required = false;
    category.functions.push_back(delay_func);
    functionLookup[delay_func.name] = delay_func;

    // RANDOM function
    StdlibFunction random_func;
    random_func.name = "RANDOM";
    random_func.category = "SYSTEM";
    random_func.signature = "INT(INT)";
    random_func.description = "Return random number from 0 to N-1";
    random_func.byteSize = 14;
    random_func.required = false;
    category.functions.push_back(random_func);
    functionLookup[random_func.name] = random_func;

    library["SYSTEM"] = category;
}

std::set<std::string> BasicStandardLibraryInjector::scanFunctionCalls(const std::string& sourceCode) {
    return FunctionCallAnalyzer::analyzeCalls(sourceCode);
}

std::vector<StdlibFunction> BasicStandardLibraryInjector::getRequiredFunctions(
    const std::set<std::string>& usedFunctions
) {
    std::set<std::string> resolved;
    resolveDependencies(usedFunctions, resolved);

    std::vector<StdlibFunction> result;
    for (const auto& funcName : resolved) {
        auto it = functionLookup.find(funcName);
        if (it != functionLookup.end()) {
            result.push_back(it->second);
        }
    }

    return result;
}

std::string BasicStandardLibraryInjector::injectLibraryCode(
    const std::string& sourceCode,
    const std::vector<StdlibFunction>& functions
) {
    std::ostringstream result;

    // Add library header comment
    result << "REM Standard Library Injection - Automatically injected functions\n";
    result << "REM Total functions: " << functions.size() << "\n";
    result << "REM Total size: " << calculateLibrarySize(functions) << " bytes\n\n";

    // Add each function with documentation
    for (const auto& func : functions) {
        result << "REM Function: " << func.name << "\n";
        result << "REM Category: " << func.category << "\n";
        result << "REM Signature: " << func.signature << "\n";
        result << "REM Description: " << func.description << "\n";
        result << "REM Size: " << func.byteSize << " bytes\n";
        result << func.name << ":\n";
        result << "REM [Implementation in assembler/bytecode]\n\n";
    }

    // Add original source
    result << "REM User source code begins here\n";
    result << sourceCode;

    return result.str();
}

std::string BasicStandardLibraryInjector::generateLibraryInit() {
    std::ostringstream init;

    init << "REM Standard Library Initialization\n";
    init << "REM Initialize random seed\n";
    init << "__stdlib_init_done = 1\n";

    return init.str();
}

const StdlibFunction* BasicStandardLibraryInjector::getFunction(const std::string& name) const {
    auto it = functionLookup.find(name);
    if (it != functionLookup.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<StdlibFunction> BasicStandardLibraryInjector::getFunctionsInCategory(
    const std::string& category
) const {
    auto it = library.find(category);
    if (it != library.end()) {
        return it->second.functions;
    }
    return {};
}

int BasicStandardLibraryInjector::calculateLibrarySize(const std::vector<StdlibFunction>& functions) const {
    int total = 0;
    for (const auto& func : functions) {
        total += func.byteSize;
    }
    return total;
}

BasicStandardLibraryInjector::LibraryStats BasicStandardLibraryInjector::getStatistics() const {
    LibraryStats stats;
    stats.totalFunctions = functionLookup.size();
    stats.categoryCount = library.size();
    stats.totalSize = 0;

    for (const auto& [category, catData] : library) {
        int catSize = 0;
        for (const auto& func : catData.functions) {
            catSize += func.byteSize;
            stats.totalSize += func.byteSize;
        }
        stats.sizeByCategory[category] = catSize;
    }

    return stats;
}

void BasicStandardLibraryInjector::resolveDependencies(
    const std::set<std::string>& functions,
    std::set<std::string>& resolved
) {
    for (const auto& func : functions) {
        if (resolved.count(func)) continue;

        resolved.insert(func);

        auto it = functionLookup.find(func);
        if (it != functionLookup.end()) {
            resolveDependencies(it->second.dependencies, resolved);
        }
    }
}

// ==================== SmartLibraryManager ====================

SmartLibraryManager::SmartLibraryManager(const LibraryConfig& config)
    : config(config), injector(BasicStandardLibraryInjector()) {}

bool SmartLibraryManager::analyzeProgramAndInjectLibrary(
    std::string& sourceCode,
    std::string& errorMessage
) {
    // Scan for function calls
    auto usedFunctions = injector.scanFunctionCalls(sourceCode);

    // Exclude configured functions
    for (const auto& excluded : config.excludeFunctions) {
        usedFunctions.erase(excluded);
    }

    // Add forced includes
    for (const auto& included : config.includeFunctions) {
        usedFunctions.insert(included);
    }

    // Get required functions with dependencies
    auto requiredFunctions = injector.getRequiredFunctions(usedFunctions);

    // Check size limit
    int libSize = injector.calculateLibrarySize(requiredFunctions);
    if (libSize > config.maxLibrarySize) {
        errorMessage = "Library size (" + std::to_string(libSize) + " bytes) exceeds limit (" +
                      std::to_string(config.maxLibrarySize) + " bytes)";
        return false;
    }

    // Generate injection report
    lastReport.success = true;
    lastReport.totalBytesAdded = libSize;
    for (const auto& func : requiredFunctions) {
        lastReport.injectedFunctions.push_back(func.name);
    }

    // Inject library code if auto-inject is enabled
    if (config.autoInject) {
        sourceCode = injector.injectLibraryCode(sourceCode, requiredFunctions);

        if (config.generateInit) {
            sourceCode = injector.generateLibraryInit() + "\n" + sourceCode;
        }
    }

    return true;
}
