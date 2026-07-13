#include "TypeInfo.hpp"
#include <stdexcept>

int TypeInfo::getTypeSize(const std::string& type, int ptrLevel, int arraySize,
                          const std::map<std::string, std::shared_ptr<TypeInfo::StructInfo>>& structs) {
    int size = 0;
    if (ptrLevel > 0) size = 2;
    else if (type == "char") size = 1;
    else if (type == "_Bool") size = 1;
    else if (type == "int") size = 2;
    else if (type == "long") size = 4;
    else if (type.length() >= 5 && type.substr(0, 5) == "enum ") size = 2;
    else if (type.substr(0, 7) == "struct " || type.substr(0, 6) == "union ") {
        std::string sName = type.substr(type.find(' ') + 1);
        if (structs.count(sName)) size = structs.at(sName)->totalSize;
        else throw std::runtime_error("Unknown struct/union type: " + sName);
    }
    if (arraySize >= 0) size *= arraySize;
    return size;
}
