#pragma once
#include <vector>
#include <cstdint>
#include <string>

class AssemblerParser;
class M65Emitter;

class AssemblerGenerator {
public:
    static std::vector<uint8_t> generate(AssemblerParser* parser, bool isPrg = true);
    static void generate(AssemblerParser* parser, M65Emitter& e, const std::string& singleSegment = "");

};
