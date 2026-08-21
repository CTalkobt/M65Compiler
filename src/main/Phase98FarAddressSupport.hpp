// Phase 98.1: Far Address Support (24-Bit Addressing)
// IR extensions for MEGA65 extended memory via MAP register

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

// Phase 98: Far address (24-bit with bank selection)
namespace phase98 {

// MAP register configuration for bank selection
struct BankConfiguration {
    unsigned bankId;          // 0-255 (which 64KB bank)
    unsigned mapRegisterValue; // Value to write to $FFF8
    std::string description;   // For diagnostics
};

// Far memory access pattern
enum class FarAccessType {
    Load,                      // lda ($addr), y
    Store,                     // sta ($addr), y
    LoadDirect,               // lda $addr (16-bit offset in bank)
    StoreDirect,              // sta $addr (16-bit offset in bank)
};

// Far pointer (24-bit address)
struct FarPointer {
    unsigned address24;        // 24-bit address ($000000-$FFFFFF)
    unsigned bankId;           // Bank ID (derived from high byte)
    unsigned bankOffset;       // Offset within 64KB bank (0-$FFFF)

    FarPointer(unsigned addr24 = 0) : address24(addr24) {
        bankId = (addr24 >> 16) & 0xFF;      // High byte is bank
        bankOffset = addr24 & 0xFFFF;        // Low 16 bits are offset
    }

    // Convert to MAP register value
    unsigned getMapValue() const {
        // MEGA65 MAP register: bits 7-4 = banks for A00000-CFFFFF
        // For now, simplified: just the bank ID
        return bankId;
    }
};

// Far memory access code sequence
struct FarAccessSequence {
    FarAccessType type;
    FarPointer target;
    std::string valueRegister;    // Register holding data (A, X, Y, Z)
    std::vector<std::string> asm; // Generated assembly lines
    int cycleCount;               // Estimated cycle count
};

// IR node for bank setup
class BankSetupNode {
public:
    explicit BankSetupNode(unsigned bank) : bankId(bank), mapValue(bank) {}

    unsigned getBankId() const { return bankId; }
    unsigned getMapValue() const { return mapValue; }
    std::string generateAssembly() const;

private:
    unsigned bankId;
    unsigned mapValue;
};

// IR node for far memory load
class FarLoadNode {
public:
    FarLoadNode(const std::string& targetReg, const FarPointer& ptr)
        : targetRegister(targetReg), pointer(ptr) {}

    const std::string& getTargetRegister() const { return targetRegister; }
    const FarPointer& getPointer() const { return pointer; }
    std::vector<std::string> generateAssembly() const;

private:
    std::string targetRegister;
    FarPointer pointer;
};

// IR node for far memory store
class FarStoreNode {
public:
    FarStoreNode(const std::string& srcReg, const FarPointer& ptr)
        : sourceRegister(srcReg), pointer(ptr) {}

    const std::string& getSourceRegister() const { return sourceRegister; }
    const FarPointer& getPointer() const { return pointer; }
    std::vector<std::string> generateAssembly() const;

private:
    std::string sourceRegister;
    FarPointer pointer;
};

// Far memory manager (handles bank allocation and caching)
class FarMemoryManager {
public:
    FarMemoryManager();

    // Bank allocation
    unsigned allocateBank(size_t sizeNeeded, const std::string& varName);
    bool isVariableInFarMemory(const std::string& varName) const;
    FarPointer getVariableAddress(const std::string& varName) const;

    // Bank caching
    void setCachedBank(unsigned bankId);
    unsigned getCachedBank() const { return cachedBank; }
    bool isBankCached(unsigned bankId) const { return bankId == cachedBank; }

    // Statistics
    int getBankCount() const { return bankAllocations.size(); }
    size_t getTotalFarMemoryUsed() const;

private:
    // Map: variable name → far address
    std::map<std::string, FarPointer> varToAddress;

    // Map: bank ID → allocated space
    std::map<unsigned, size_t> bankAllocations;

    // Currently cached bank in MAP register (-1 = none)
    int cachedBank = -1;

    // Total banks available (MEGA65 has 256 banks)
    static constexpr unsigned TOTAL_BANKS = 256;
    static constexpr unsigned BANK_SIZE = 0x10000;  // 64KB per bank
};

} // namespace phase98

