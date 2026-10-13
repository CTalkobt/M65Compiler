#include "BasicDocGenerator.hpp"
#include <sstream>
#include <algorithm>

std::string BasicDocGenerator::generateMarkdown(const std::string& source, const std::string& title) {
    entries.clear();
    extract(source, true);
    return renderMarkdown(title);
}

void BasicDocGenerator::extract(const std::string& source, bool /* useLabels */) {
    std::istringstream iss(source);
    std::string line;
    int lineNum = 10;
    int increment = 10;
    std::vector<std::string> pendingComments;

    while (std::getline(iss, line)) {
        // Skip empty lines
        if (line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }

        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            continue;
        }

        std::string trimmed = line.substr(start);

        // A comment: a ' line, or a # line that is not a preprocessor
        // directive (#define, #include, ...). Directives are skipped.
        if (trimmed[0] == '#') {
            std::string word;
            for (size_t k = 1; k < trimmed.size() && std::isalpha((unsigned char)trimmed[k]); k++) word += trimmed[k];
            static const char* directives[] = {"include", "define", "undef", "ifdef", "ifndef",
                                               "elif", "else", "endif"};
            if (std::find(std::begin(directives), std::end(directives), word) != std::end(directives))
                continue;
        }
        if (trimmed[0] == '#' || trimmed[0] == '\'') {
            // Extract comment text (remove the # or ' and a leading space)
            std::string comment = trimmed.substr(1);
            if (!comment.empty() && comment[0] == ' ') {
                comment = comment.substr(1);
            }
            pendingComments.push_back(comment);
            continue;
        }

        // Check for label (ends with :)
        size_t colonPos = trimmed.find(':');
        if (colonPos != std::string::npos && colonPos > 0) {
            // Check if it's a label (identifier before colon)
            bool isLabel = std::all_of(trimmed.begin(), trimmed.begin() + colonPos,
                [](char c) { return std::isalnum(c) || c == '_'; });

            if (isLabel && (colonPos + 1 >= trimmed.length() ||
                std::isspace(trimmed[colonPos + 1]))) {
                DocumentationEntry entry;
                entry.name = trimmed.substr(0, colonPos);
                entry.type = "label";
                std::string up = entry.name;
                std::transform(up.begin(), up.end(), up.begin(), ::toupper);
                auto ll = labelLines.find(up);
                entry.lineNumber = ll != labelLines.end() ? ll->second : lineNum;
                entry.comments = pendingComments;
                entries.push_back(entry);
                pendingComments.clear();
            }
        }

        // Check for subroutine call (gosub)
        if (trimmed.find("gosub") != std::string::npos ||
            trimmed.find("GOSUB") != std::string::npos) {
            DocumentationEntry entry;
            entry.name = "Subroutine call at line " + std::to_string(lineNum);
            entry.type = "subroutine_call";
            entry.lineNumber = lineNum;
            entry.comments = pendingComments;
            pendingComments.clear();
        }

        lineNum += increment;
    }
}

std::string BasicDocGenerator::renderMarkdown(const std::string& title) const {
    std::ostringstream md;

    md << "# " << title << "\n\n";
    md << "## Program Structure\n\n";

    int sectionCount = 0;
    for (const auto& entry : entries) {
        if (entry.type == "label") {
            sectionCount++;
            md << "### " << sectionCount << ". " << entry.name
               << " (Line " << entry.lineNumber << ")\n\n";

            if (!entry.comments.empty()) {
                for (const auto& comment : entry.comments) {
                    md << comment << "\n";
                }
                md << "\n";
            } else {
                md << "*No description*\n\n";
            }
        }
    }

    if (sectionCount == 0) {
        md << "No labeled sections found.\n\n";
    }

    return md.str();
}
