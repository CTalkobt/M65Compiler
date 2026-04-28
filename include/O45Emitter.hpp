#pragma once
#include <vector>
#include <string>
#include <cstdint>

class AssemblerParser;

// Produces a .o45 relocatable object file from a fully-assembled parser.
// Call after pass1() and pass2() have completed.
//
// The emitter:
//   1. Extracts per-segment bodies (text, data) from the flat binary
//   2. Scans statements for relocatable operands (absolute addresses
//      referencing symbols in relocatable segments or .extern symbols)
//   3. Builds import/export tables from .extern/.global directives
//   4. Packages everything via O45Writer
//
// Returns the complete .o45 file as a byte vector.
std::vector<uint8_t> emitO45(AssemblerParser& parser, const std::string& asmVersion = "ca45");
