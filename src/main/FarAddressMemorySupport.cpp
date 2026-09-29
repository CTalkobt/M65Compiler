// Phase 98.1: Far Address Support Implementation
// Code generation for MEGA65 24-bit far memory access

#include "FarAddressMemorySupport.hpp"
#include <sstream>
#include <iomanip>

namespace phase98 {

// BankSetupNode: Generate MAP register setup code
std::string BankSetupNode::generateAssembly() const {
    std::ostringstream oss;
    // MEGA65 MAP register is at $FFF8
    // Simple bank setup: lda #bankId; sta $FFF8
    oss << std::hex << std::setfill('0');
    oss << "    lda #$" << std::setw(2) << mapValue << "\n";
    oss << "    sta $FFF8  ; Set MAP register for bank\n";
    return oss.str();
}

// FarLoadNode: Generate far memory load sequence
std::vector<std::string> FarLoadNode::generateAssembly() const {
    std::vector<std::string> asmLines;
    std::ostringstream oss;

    // Generate far load sequence:
    // 1. Setup bank (if not cached)
    // 2. Load from bank-relative address

    unsigned bank = pointer.bankId;
    unsigned offset = pointer.bankOffset;

    oss << std::hex << std::setfill('0');

    // Bank setup
    oss << "    lda #$" << std::setw(2) << bank;
    asmLines.push_back(oss.str()); oss.str(""); oss.clear();

    asmLines.push_back("    sta $FFF8  ; Set MAP register");

    // Load from bank-relative address
    // Use indirect addressing: lda ($addr),y where addr contains offset
    oss << "    lda #$" << std::setw(2) << (offset & 0xFF);
    asmLines.push_back(oss.str()); asmLines.push_back("    sta $20    ; Store low byte of offset");
    oss.str(""); oss.clear();

    oss << "    lda #$" << std::setw(2) << ((offset >> 8) & 0xFF);
    asmLines.push_back(oss.str()); asmLines.push_back("    sta $21    ; Store high byte of offset");
    oss.str(""); oss.clear();

    asmLines.push_back("    ldy #0");
    asmLines.push_back("    lda ($20),y  ; Load from far memory");

    if (targetRegister != "A") {
        oss << "    ld" << std::tolower(targetRegister[0]) << " #0 ; Move to target register (placeholder)";
        asmLines.push_back(oss.str());
    }

    return asmLines;
}

// FarStoreNode: Generate far memory store sequence
std::vector<std::string> FarStoreNode::generateAssembly() const {
    std::vector<std::string> asmLines;
    std::ostringstream oss;

    unsigned bank = pointer.bankId;
    unsigned offset = pointer.bankOffset;

    oss << std::hex << std::setfill('0');

    // Bank setup
    oss << "    lda #$" << std::setw(2) << bank;
    asmLines.push_back(oss.str()); oss.str(""); oss.clear();

    asmLines.push_back("    sta $FFF8  ; Set MAP register");

    // Setup offset in ZP
    oss << "    lda #$" << std::setw(2) << (offset & 0xFF);
    asmLines.push_back(oss.str()); asmLines.push_back("    sta $20");
    oss.str(""); oss.clear();

    oss << "    lda #$" << std::setw(2) << ((offset >> 8) & 0xFF);
    asmLines.push_back(oss.str()); asmLines.push_back("    sta $21");
    oss.str(""); oss.clear();

    // Load value from source register
    if (sourceRegister != "A") {
        oss << "    ld" << std::tolower(sourceRegister[0]);
        asmLines.push_back(oss.str());
        oss.str(""); oss.clear();
    }

    // Store to far memory
    asmLines.push_back("    ldy #0");
    asmLines.push_back("    sta ($20),y  ; Store to far memory");

    return asmLines;
}

// FarMemoryManager implementation
FarMemoryManager::FarMemoryManager() = default;

unsigned FarMemoryManager::allocateBank(size_t sizeNeeded, const std::string& varName) {
    // Allocate next available bank with sufficient space
    unsigned bank = 0;

    for (bank = 1; bank < TOTAL_BANKS; bank++) {
        size_t used = bankAllocations[bank];
        if (used + sizeNeeded <= BANK_SIZE) {
            // Allocate in this bank
            unsigned offset = used;
            FarPointer addr24((bank << 16) | offset);
            varToAddress[varName] = addr24;
            bankAllocations[bank] = used + sizeNeeded;
            return bank;
        }
    }

    // Error: no space available
    return 0;
}

bool FarMemoryManager::isVariableInFarMemory(const std::string& varName) const {
    return varToAddress.find(varName) != varToAddress.end();
}

FarPointer FarMemoryManager::getVariableAddress(const std::string& varName) const {
    auto it = varToAddress.find(varName);
    if (it != varToAddress.end()) {
        return it->second;
    }
    return FarPointer(0);  // Invalid address
}

size_t FarMemoryManager::getTotalFarMemoryUsed() const {
    size_t total = 0;
    for (const auto& [bank, used] : bankAllocations) {
        total += used;
    }
    return total;
}

} // namespace phase98

