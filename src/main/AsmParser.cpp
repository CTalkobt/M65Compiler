#include "AsmParser.hpp"

ParserRegistry& ParserRegistry::getInstance() {
    static ParserRegistry instance;
    return instance;
}

void ParserRegistry::registerParser(const std::string& format, ParserFactory factory) {
    parsers_[format] = factory;
}

std::unique_ptr<AsmParser> ParserRegistry::createParser(const std::string& format) {
    auto it = parsers_.find(format);
    if (it != parsers_.end()) {
        return std::unique_ptr<AsmParser>(it->second());
    }
    return nullptr;
}

std::vector<std::string> ParserRegistry::getAvailableParsers() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : parsers_) {
        result.push_back(name);
    }
    return result;
}
