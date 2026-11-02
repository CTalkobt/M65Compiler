#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "AssemblerTypes.hpp"

class AssemblerOpcodeDatabase {
public:
    static uint8_t getOpcode(const std::string& mnemonic, AddressingMode mode);
    static std::vector<AddressingMode> getValidAddressingModes(const std::string& mnemonic);
    static bool isValidMode(const std::string& mnemonic, AddressingMode mode);
    static std::string AddressingModeToString(AddressingMode mode);

    static const std::map<std::pair<std::string, AddressingMode>, uint8_t>& getOpcodeMap();

    // The NMOS 6502's (6510's) documented instructions (#384, .cpu 6502): the
    // 45GS02 encodes them the same. ($EA, the 6502's NOP, is ca45's eom.)
    static bool isOn6502(const std::string& mnemonic, AddressingMode mode);
};
