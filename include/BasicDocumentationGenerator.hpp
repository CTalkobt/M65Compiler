#ifndef BASICDOCUMENTATIONGENERATOR_HPP
#define BASICDOCUMENTATIONGENERATOR_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

// Enhanced documentation generator with advanced features
class BasicDocumentationGenerator {
public:
    struct Symbol {
        std::string name;
        std::string type;  // "LABEL", "VARIABLE", "FUNCTION"
        int lineNumber;
        std::string description;
    };

    struct CrossReference {
        std::string symbol;
        std::vector<int> lineNumbers;  // Where referenced
    };

    struct DocumentationOptions {
        bool includeSymbols = true;
        bool includeCrossReferences = true;
        bool includeLineNumbers = true;
        bool generateTableOfContents = true;
        bool generateIndex = true;
        bool includeSourcePreview = true;
    };

    BasicDocumentationGenerator();

    // Generate comprehensive documentation
    std::string generateFullDocumentation(
        const std::string& sourceCode,
        const std::string& filename,
        const DocumentationOptions& options
    );

    // Generate table of contents (labels/sections)
    std::string generateTableOfContents();

    // Generate symbol table
    std::string generateSymbolTable();

    // Generate cross-reference index
    std::string generateCrossReferences();

    // Generate sorted index of all identifiers
    std::string generateIndex();

    // Add symbol information
    void addSymbol(const Symbol& symbol);

    // Find all symbols of a type
    std::vector<Symbol> findSymbolsByType(const std::string& type) const;

    // Generate call graph (which labels call which)
    std::string generateCallGraph();

    // Extract and document procedures/subroutines
    std::string documentProcedures(const std::string& sourceCode);

    // Generate statistics about the program
    struct ProgramStatistics {
        int totalLines;
        int commentLines;
        int blankLines;
        int labelCount;
        int variableCount;
        int subroutineCount;
        double commentPercentage;
    };

    ProgramStatistics calculateStatistics(const std::string& sourceCode);

    // Generate statistics section
    std::string generateStatistics();

private:
    std::vector<Symbol> symbols;
    std::map<std::string, CrossReference> crossReferences;
    ProgramStatistics stats;

    // Helper methods
    void extractSymbols(const std::string& sourceCode);
    void buildCrossReferences(const std::string& sourceCode);

    std::vector<std::string> splitLines(const std::string& text) const;
    std::string escapeMarkdown(const std::string& text) const;
    bool isLabel(const std::string& line) const;
    bool isVariable(const std::string& line) const;
};

// Documentation formatter - converts raw docs to different formats
class DocumentationFormatter {
public:
    enum class Format {
        MARKDOWN,
        HTML,
        PLAIN_TEXT,
        RESTRUCTURED_TEXT
    };

    // Format documentation
    static std::string format(
        const std::string& documentation,
        Format targetFormat
    );

    // Generate HTML with styling
    static std::string formatAsHTML(const std::string& markdown);

    // Generate RST (ReStructuredText)
    static std::string formatAsRST(const std::string& markdown);

    // Generate plain text (strip formatting)
    static std::string formatAsPlainText(const std::string& markdown);

private:
    static std::string markdownToHTML(const std::string& md);
    static std::string markdownToRST(const std::string& md);
};

#endif  // BASICDOCUMENTATIONGENERATOR_HPP
