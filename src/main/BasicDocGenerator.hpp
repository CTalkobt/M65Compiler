#pragma once

#include <string>
#include <vector>
#include <map>

struct DocumentationEntry {
    std::string name;
    std::string type;  // "label", "subroutine", "section"
    int lineNumber;
    std::vector<std::string> comments;
};

class BasicDocGenerator {
public:
    // Generate documentation from BASIC source
    std::string generateMarkdown(const std::string& source, const std::string& title = "BASIC Program");

    // Get extracted documentation entries
    const std::vector<DocumentationEntry>& getEntries() const { return entries; }

private:
    std::vector<DocumentationEntry> entries;

    void extract(const std::string& source, bool useLabels);
    std::string renderMarkdown(const std::string& title) const;
};
