#include "PETSCIIEncoder.hpp"
#include <cctype>

static const std::unordered_map<std::string, char>& escTable();

PETSCIIEncoder::PETSCIIEncoder() {
    initializeColorCodes();
    initializeControlCodes();
    escSequences = escTable();
}

void PETSCIIEncoder::initializeColorCodes() {
    colorCodes = {
        {"black", 0x90},
        {"white", 0x05},
        {"red", 0x1C},
        {"cyan", 0x9F},
        {"magenta", 0x9C},
        {"purple", 0x9C},
        {"green", 0x1E},
        {"blue", 0x1F},
        {"yellow", 0x9E},
        {"orange", 0x81},
        {"brown", 0x95},
        {"light-red", 0x96}, {"lred", 0x96},
        {"dark-gray", 0x97}, {"dark-grey", 0x97}, {"dgray", 0x97},
        {"gray", 0x98}, {"grey", 0x98},
        {"light-green", 0x99}, {"lgreen", 0x99},
        {"light-blue", 0x9A}, {"lblue", 0x9A},
        {"light-gray", 0x9B}, {"light-grey", 0x9B}, {"lgray", 0x9B},
    };
}

void PETSCIIEncoder::initializeControlCodes() {
    controlCodes = {
        {"home", 0x13},
        {"clear", 0x93},
        // MEGA65 control codes (The MEGA65 Book, appendix C)
        {"return", 0x0D},
        {"linefeed", 0x0A}, {"lf", 0x0A},
        {"lower", 0x0E}, {"lowercase", 0x0E},
        {"upper", 0x8E}, {"uppercase", 0x8E},
        {"case-lock", 0x0B}, {"lock", 0x0B},          // disable SHIFT+C= case switching
        {"case-unlock", 0x0C}, {"unlock", 0x0C},      // enable it
        {"flash", 0x0F}, {"blink", 0x0F},
        {"flash-off", 0x8F}, {"blink-off", 0x8F},
        {"tab-left", 0x1A},                           // CTRL+Z
        {"word-left", 0x15},                          // CTRL+U
        {"word-right", 0x17},                         // CTRL+W
        {"tab-set", 0x18},                            // CTRL+X: set/clear a tab stop
        {"esc", 0x1B},
        {"back", 0x14},
        {"delete", 0x14},
        {"del", 0x14},
        {"insert", 0x94},
        {"ins", 0x94},
        {"reverse", 0x12},
        {"rev", 0x12},
        {"under", 0x02},
        {"underline", 0x02},
        {"under-off", 0x82},
        {"off", 0x92},
        {"rvs-off", 0x92},
        {"reverse-off", 0x92},
        {"bell", 0x07},
        {"up", 0x91},
        {"down", 0x11},
        {"left", 0x9D},
        {"right", 0x1D},
        {"tab", 0x09},
        {"crsr-up", 0x91},
        {"crsr-down", 0x11},
        {"crsr-left", 0x9D},
        {"crsr-right", 0x1D},
        {"f1", 0x85},
        {"f2", 0x89},
        {"f3", 0x86},
        {"f4", 0x8A},
        {"f5", 0x87},
        {"f6", 0x8B},
        {"f7", 0x88},
        {"f8", 0x8C},
        {"shift-f1", 0x89},
        {"shift-f3", 0x8A},
        {"shift-f5", 0x8B},
        {"shift-f7", 0x8C},
    };
}

// MEGA65 ESC sequences (The MEGA65 Book, appendix C): ESC, then a key
static const std::unordered_map<std::string, char>& escTable() {
    static const std::unordered_map<std::string, char> t = {
        {"clear-eos", '@'},                  // clear from the cursor to the end of the screen
        {"insert-on", 'A'},                  // auto-insert mode
        {"window-bottom", 'B'},              // bottom-right of the window at the cursor
        {"insert-off", 'C'},                 // overwrite mode
        {"delete-line", 'D'},
        {"cursor-solid", 'E'},               // non-flashing cursor
        {"cursor-flash", 'F'},
        {"bell-on", 'G'},
        {"bell-off", 'H'},
        {"insert-line", 'I'},
        {"line-start", 'J'},
        {"line-end", 'K'},
        {"scroll-on", 'L'},
        {"scroll-off", 'M'},
        {"normal", 'O'},                     // cancel quote, reverse, underline and flash modes
        {"erase-sol", 'P'},                  // erase to the start of the line
        {"erase-eol", 'Q'},                  // erase to the end of the line
        {"colors-high", 'S'}, {"colours-high", 'S'},   // colour range 16-31
        {"window-top", 'T'},                 // top-left of the window at the cursor
        {"colors-low", 'U'}, {"colours-low", 'U'},     // colour range 0-15
        {"scroll-up", 'V'},
        {"scroll-down", 'W'},
        {"40-80", 'X'}, {"80col", 'X'}, {"40col", 'X'},   // toggle 40/80 columns (clears)
        {"tabs-default", 'Y'},               // a tab stop every 8 columns
        {"tabs-clear", 'Z'},
    };
    return t;
}

bool PETSCIIEncoder::escapeBytes(const std::string& name, std::vector<uint8_t>& out) const {
    // {$xx}: the byte xx (hex)
    if (name.size() == 3 && name[0] == '$' && std::isxdigit((unsigned char)name[1]) &&
        std::isxdigit((unsigned char)name[2])) {
        out.push_back((uint8_t)std::stoi(name.substr(1), nullptr, 16));
        return true;
    }
    if (uint8_t c = getColorCode(name)) { out.push_back(c); return true; }
    if (uint8_t c = getControlCode(name)) { out.push_back(c); return true; }
    auto it = escSequences.find(name);
    if (it != escSequences.end()) {
        out.push_back(0x1B);
        out.push_back((uint8_t)it->second);
        return true;
    }
    // {esc-k}: ESC then the key k (a letter, digit or @)
    if (name.size() == 5 && name.compare(0, 4, "esc-") == 0 &&
        (std::isalnum((unsigned char)name[4]) || name[4] == '@')) {
        out.push_back(0x1B);
        out.push_back((uint8_t)std::toupper((unsigned char)name[4]));
        return true;
    }
    return false;
}

std::vector<uint8_t> PETSCIIEncoder::encode(const std::string& text) {
    std::vector<uint8_t> result;
    size_t i = 0;

    while (i < text.length()) {
        if (text[i] == '{') {
            size_t end = text.find('}', i);
            if (end != std::string::npos) {
                std::string escape = text.substr(i + 1, end - i - 1);

                if (escapeBytes(escape, result)) {
                    // done
                } else {
                    result.push_back(charToPetscii('{'));
                    for (char c : escape) {
                        result.push_back(charToPetscii(c));
                    }
                    result.push_back(charToPetscii('}'));
                }
                i = end + 1;
            } else {
                result.push_back(charToPetscii(text[i]));
                i++;
            }
        } else {
            result.push_back(charToPetscii(text[i]));
            i++;
        }
    }

    return result;
}

std::string PETSCIIEncoder::decode(const std::vector<uint8_t>& bytes) {
    std::string result;
    for (uint8_t b : bytes) {
        result += petsciiToChar(b);
    }
    return result;
}

uint8_t PETSCIIEncoder::charToPetscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<uint8_t>(c);
    } else if (c >= 'a' && c <= 'z') {
        return static_cast<uint8_t>(c - 32);
    } else if (c >= '0' && c <= '9') {
        return static_cast<uint8_t>(c);
    } else if (c == ' ') {
        return 0x20;
    } else if (c == '!') {
        return 0x21;
    } else if (c == '"') {
        return 0x22;
    } else if (c == '#') {
        return 0x23;
    } else if (c == '$') {
        return 0x24;
    } else if (c == '%') {
        return 0x25;
    } else if (c == '&') {
        return 0x26;
    } else if (c == '\'') {
        return 0x27;
    } else if (c == '(') {
        return 0x28;
    } else if (c == ')') {
        return 0x29;
    } else if (c == '*') {
        return 0x2A;
    } else if (c == '+') {
        return 0x2B;
    } else if (c == ',') {
        return 0x2C;
    } else if (c == '-') {
        return 0x2D;
    } else if (c == '.') {
        return 0x2E;
    } else if (c == '/') {
        return 0x2F;
    } else if (c == ':') {
        return 0x3A;
    } else if (c == ';') {
        return 0x3B;
    } else if (c == '<') {
        return 0x3C;
    } else if (c == '=') {
        return 0x3D;
    } else if (c == '>') {
        return 0x3E;
    } else if (c == '?') {
        return 0x3F;
    } else if (c == '@') {
        return 0x40;
    } else if (c == '[') {
        return 0x5B;
    } else if (c == '\\') {
        return 0x5C;
    } else if (c == ']') {
        return 0x5D;
    } else if (c == '^') {
        return 0x5E;
    } else if (c == '_') {
        return 0x5F;
    }

    return 0x20;
}

char PETSCIIEncoder::petsciiToChar(uint8_t b) {
    if ((b >= 0x41 && b <= 0x5A) || (b >= 0x61 && b <= 0x7A)) {
        return static_cast<char>(b);
    } else if (b >= 0x30 && b <= 0x39) {
        return static_cast<char>(b);
    } else if (b == 0x20) {
        return ' ';
    } else if (b >= 0x21 && b <= 0x2F) {
        return static_cast<char>(b);
    } else if (b >= 0x3A && b <= 0x40) {
        return static_cast<char>(b);
    } else if (b >= 0x5B && b <= 0x5F) {
        return static_cast<char>(b);
    }

    return '?';
}

uint8_t PETSCIIEncoder::getColorCode(const std::string& colorName) const {
    auto it = colorCodes.find(colorName);
    if (it != colorCodes.end()) {
        return it->second;
    }
    return 0;
}

uint8_t PETSCIIEncoder::getControlCode(const std::string& controlName) const {
    auto it = controlCodes.find(controlName);
    if (it != controlCodes.end()) {
        return it->second;
    }
    return 0;
}
