#pragma once
#include <vector>
#include <cstdint>
#include <string>

class AssemblerParser;
class M65Emitter;

class AssemblerGenerator {
public:
    static std::vector<uint8_t> generate(AssemblerParser* parser, bool isPrg = true);
    // A flat binary's pieces: where each starts in the emitter's bytes, the
    // address it belongs at and its segment; a floating piece (a segment with
    // no .org of its own) goes after the placed ones (#715)
    struct Piece { size_t index; uint32_t address; std::string segment; bool floating = false; };
    static void generate(AssemblerParser* parser, M65Emitter& e, const std::string& singleSegment = "",
                         std::vector<Piece>* pieces = nullptr);

};
