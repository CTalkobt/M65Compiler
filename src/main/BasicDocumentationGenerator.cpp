#include "BasicDocumentationGenerator.hpp"
#include <sstream>
#include <algorithm>
#include <regex>
#include <iomanip>

BasicDocumentationGenerator::BasicDocumentationGenerator() {
    stats = {0, 0, 0, 0, 0, 0, 0.0};
}

std::string BasicDocumentationGenerator::generateFullDocumentation(
    const std::string& sourceCode,
    const std::string& filename,
    const DocumentationOptions& options
) {
    // Extract information from source
    extractSymbols(sourceCode);
    buildCrossReferences(sourceCode);
    calculateStatistics(sourceCode);

    std::ostringstream doc;

    // Header
    doc << "# Documentation: " << filename << "\n\n";
    doc << "*Auto-generated documentation for BASIC program*\n\n";

    // Table of Contents
    if (options.generateTableOfContents) {
        doc << "## Table of Contents\n\n";
        auto toc = generateTableOfContents();
        if (!toc.empty()) {
            doc << toc << "\n\n";
        }
    }

    // Statistics
    doc << "## Program Statistics\n\n";
    doc << generateStatistics() << "\n\n";

    // Symbol Table
    if (options.includeSymbols) {
        doc << "## Symbol Table\n\n";
        doc << generateSymbolTable() << "\n\n";
    }

    // Cross References
    if (options.includeCrossReferences) {
        doc << "## Cross References\n\n";
        doc << generateCrossReferences() << "\n\n";
    }

    // Index
    if (options.generateIndex) {
        doc << "## Index\n\n";
        doc << generateIndex() << "\n\n";
    }

    // Procedures
    doc << "## Procedures and Subroutines\n\n";
    doc << documentProcedures(sourceCode) << "\n\n";

    doc << "---\n";
    doc << "*Generated on " << __DATE__ << " at " << __TIME__ << "*\n";

    return doc.str();
}

std::string BasicDocumentationGenerator::generateTableOfContents() {
    std::ostringstream toc;

    // Find all labels (procedures)
    for (const auto& sym : symbols) {
        if (sym.type == "LABEL") {
            toc << "- " << sym.name << " (Line " << sym.lineNumber << ")\n";
        }
    }

    return toc.str();
}

std::string BasicDocumentationGenerator::generateSymbolTable() {
    std::ostringstream table;

    table << "| Symbol | Type | Line | Description |\n";
    table << "|--------|------|------|-------------|\n";

    for (const auto& sym : symbols) {
        table << "| `" << sym.name << "` | " << sym.type << " | "
              << sym.lineNumber << " | " << sym.description << " |\n";
    }

    return table.str();
}

std::string BasicDocumentationGenerator::generateCrossReferences() {
    std::ostringstream xref;

    for (const auto& [symbol, ref] : crossReferences) {
        xref << "- **" << symbol << "** referenced at lines: ";
        for (size_t i = 0; i < ref.lineNumbers.size(); i++) {
            xref << ref.lineNumbers[i];
            if (i < ref.lineNumbers.size() - 1) {
                xref << ", ";
            }
        }
        xref << "\n";
    }

    return xref.str();
}

std::string BasicDocumentationGenerator::generateIndex() {
    std::vector<std::string> allNames;

    for (const auto& sym : symbols) {
        allNames.push_back(sym.name);
    }

    std::sort(allNames.begin(), allNames.end());
    allNames.erase(std::unique(allNames.begin(), allNames.end()), allNames.end());

    std::ostringstream index;
    for (const auto& name : allNames) {
        index << "- " << name << "\n";
    }

    return index.str();
}

std::string BasicDocumentationGenerator::generateStatistics() {
    std::ostringstream stats;

    stats << "- **Total Lines:** " << this->stats.totalLines << "\n";
    stats << "- **Comment Lines:** " << this->stats.commentLines << "\n";
    stats << "- **Blank Lines:** " << this->stats.blankLines << "\n";
    stats << "- **Code Lines:** " << (this->stats.totalLines - this->stats.commentLines - this->stats.blankLines) << "\n";
    stats << "- **Labels/Procedures:** " << this->stats.labelCount << "\n";
    stats << "- **Variables:** " << this->stats.variableCount << "\n";
    stats << "- **Subroutines:** " << this->stats.subroutineCount << "\n";
    stats << "- **Comment Percentage:** " << std::fixed << std::setprecision(1) << this->stats.commentPercentage << "%\n";

    return stats.str();
}

void BasicDocumentationGenerator::addSymbol(const Symbol& symbol) {
    symbols.push_back(symbol);
}

std::vector<BasicDocumentationGenerator::Symbol> BasicDocumentationGenerator::findSymbolsByType(const std::string& type) const {
    std::vector<Symbol> result;

    for (const auto& sym : symbols) {
        if (sym.type == type) {
            result.push_back(sym);
        }
    }

    return result;
}

std::string BasicDocumentationGenerator::generateCallGraph() {
    std::ostringstream graph;

    // Group by label
    auto labels = findSymbolsByType("LABEL");
    for (const auto& label : labels) {
        auto refs = crossReferences.find(label.name);
        if (refs != crossReferences.end() && !refs->second.lineNumbers.empty()) {
            graph << label.name << " ← called from lines: ";
            for (size_t i = 0; i < refs->second.lineNumbers.size(); i++) {
                graph << refs->second.lineNumbers[i];
                if (i < refs->second.lineNumbers.size() - 1) {
                    graph << ", ";
                }
            }
            graph << "\n";
        }
    }

    return graph.str();
}

std::string BasicDocumentationGenerator::documentProcedures(const std::string&) {
    std::ostringstream doc;

    auto labels = findSymbolsByType("LABEL");
    for (const auto& label : labels) {
        doc << "### " << label.name << "\n\n";
        doc << "**Line:** " << label.lineNumber << "\n";

        auto refs = crossReferences.find(label.name);
        if (refs != crossReferences.end()) {
            doc << "**Called from lines:** ";
            for (size_t i = 0; i < refs->second.lineNumbers.size(); i++) {
                doc << refs->second.lineNumbers[i];
                if (i < refs->second.lineNumbers.size() - 1) {
                    doc << ", ";
                }
            }
            doc << "\n";
        }

        doc << "\n";
    }

    return doc.str();
}

BasicDocumentationGenerator::ProgramStatistics BasicDocumentationGenerator::calculateStatistics(const std::string& sourceCode) {
    auto lines = splitLines(sourceCode);

    stats.totalLines = lines.size();
    stats.commentLines = 0;
    stats.blankLines = 0;
    stats.labelCount = 0;
    stats.variableCount = 0;
    stats.subroutineCount = 0;

    for (const auto& line : lines) {
        // Trim whitespace
        auto trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

        if (trimmed.empty()) {
            stats.blankLines++;
        } else if (trimmed.substr(0, 3) == "REM") {
            stats.commentLines++;
        } else if (trimmed.find(':') != std::string::npos) {
            stats.labelCount++;
        } else if (trimmed.find("GOSUB") != std::string::npos) {
            stats.subroutineCount++;
        } else if (trimmed.find('=') != std::string::npos) {
            stats.variableCount++;
        }
    }

    int codeLines = stats.totalLines - stats.commentLines - stats.blankLines;
    if (codeLines > 0) {
        stats.commentPercentage = (100.0 * stats.commentLines) / codeLines;
    } else {
        stats.commentPercentage = 0.0;
    }

    return stats;
}

void BasicDocumentationGenerator::extractSymbols(const std::string& sourceCode) {
    auto lines = splitLines(sourceCode);

    int lineNum = 10;
    for (const auto& line : lines) {
        auto trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));

        // Check for label
        size_t colonPos = trimmed.find(':');
        if (colonPos != std::string::npos && colonPos > 0) {
            std::string labelName = trimmed.substr(0, colonPos);
            Symbol sym;
            sym.name = labelName;
            sym.type = "LABEL";
            sym.lineNumber = lineNum;
            sym.description = "";
            symbols.push_back(sym);
        }

        lineNum += 10;
    }
}

void BasicDocumentationGenerator::buildCrossReferences(const std::string& sourceCode) {
    auto lines = splitLines(sourceCode);

    int lineNum = 10;
    for (const auto& line : lines) {
        // Look for GOSUB and GOTO
        std::regex gosubPattern("GOSUB\\s+(\\w+)");
        std::regex gotoPattern("GOTO\\s+(\\w+)");
        std::smatch match;

        if (std::regex_search(line, match, gosubPattern)) {
            std::string target = match[1].str();
            crossReferences[target].lineNumbers.push_back(lineNum);
        }

        if (std::regex_search(line, match, gotoPattern)) {
            std::string target = match[1].str();
            crossReferences[target].lineNumbers.push_back(lineNum);
        }

        lineNum += 10;
    }
}

std::vector<std::string> BasicDocumentationGenerator::splitLines(const std::string& text) const {
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string line;

    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    return lines;
}

std::string BasicDocumentationGenerator::escapeMarkdown(const std::string& text) const {
    std::string result = text;
    // Escape special markdown characters
    std::regex specialChars(R"([\[\]`*_])");
    result = std::regex_replace(result, specialChars, "\\$&");
    return result;
}

bool BasicDocumentationGenerator::isLabel(const std::string& line) const {
    return line.find(':') != std::string::npos;
}

bool BasicDocumentationGenerator::isVariable(const std::string& line) const {
    return line.find('=') != std::string::npos && line.find(':') == std::string::npos;
}

// ==================== DocumentationFormatter ====================

std::string DocumentationFormatter::format(const std::string& documentation, Format targetFormat) {
    switch (targetFormat) {
        case Format::MARKDOWN:
            return documentation;  // Already in markdown
        case Format::HTML:
            return formatAsHTML(documentation);
        case Format::PLAIN_TEXT:
            return formatAsPlainText(documentation);
        case Format::RESTRUCTURED_TEXT:
            return formatAsRST(documentation);
        default:
            return documentation;
    }
}

std::string DocumentationFormatter::formatAsHTML(const std::string& markdown) {
    std::ostringstream html;

    html << "<!DOCTYPE html>\n";
    html << "<html>\n<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<title>BASIC Documentation</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    html << "table { border-collapse: collapse; width: 100%; }\n";
    html << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    html << "th { background-color: #4CAF50; color: white; }\n";
    html << "code { background-color: #f4f4f4; padding: 2px 5px; }\n";
    html << "</style>\n";
    html << "</head>\n<body>\n";
    html << markdown;  // In production, convert markdown to HTML
    html << "</body>\n</html>\n";

    return html.str();
}

std::string DocumentationFormatter::formatAsPlainText(const std::string& markdown) {
    std::string text = markdown;

    // Remove markdown formatting
    std::regex headings(R"(^#+\s+)");
    std::regex boldItalic(R"([\*_]{1,2}([^\*_]+)[\*_]{1,2})");
    std::regex links(R"(\[([^\]]+)\]\([^\)]+\))");
    std::regex code(R"(`([^`]+)`)");

    text = std::regex_replace(text, headings, "");
    text = std::regex_replace(text, boldItalic, "$1");
    text = std::regex_replace(text, links, "$1");
    text = std::regex_replace(text, code, "$1");

    return text;
}

std::string DocumentationFormatter::formatAsRST(const std::string& markdown) {
    std::ostringstream rst;
    std::istringstream input(markdown);
    std::string line;

    while (std::getline(input, line)) {
        // Convert markdown headers to RST
        if (line.find("# ") == 0) {
            std::string title = line.substr(2);
            rst << title << "\n";
            rst << std::string(title.length(), '=') << "\n\n";
        } else if (line.find("## ") == 0) {
            std::string title = line.substr(3);
            rst << title << "\n";
            rst << std::string(title.length(), '-') << "\n\n";
        } else {
            rst << line << "\n";
        }
    }

    return rst.str();
}
