#include "AsmWriter.hpp"

WriterRegistry& WriterRegistry::getInstance() {
    static WriterRegistry instance;
    return instance;
}

void WriterRegistry::registerWriter(const std::string& format, WriterFactory factory) {
    writers_[format] = factory;
}

std::unique_ptr<AsmWriter> WriterRegistry::createWriter(const std::string& format) {
    auto it = writers_.find(format);
    if (it != writers_.end()) {
        return std::unique_ptr<AsmWriter>(it->second());
    }
    return nullptr;
}

std::vector<std::string> WriterRegistry::getAvailableWriters() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : writers_) {
        result.push_back(name);
    }
    return result;
}
