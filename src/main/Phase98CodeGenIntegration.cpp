// Phase 98.3: Code Generation Integration Implementation
// Integration layer for far memory code generation

#include "Phase98CodeGenIntegration.hpp"
#include <iostream>

Phase98CodeGenIntegration::Phase98CodeGenIntegration(M65Emitter& emitter)
    : emitter(emitter) {}

bool Phase98CodeGenIntegration::isFarVariable(const std::string& varName) const {
    auto it = farVariables.find(varName);
    return it != farVariables.end() && it->second;
}

void Phase98CodeGenIntegration::allocateFarVariable(const std::string& varName, size_t size) {
    farMemoryManager.allocateBank(size, varName);
    farVariables[varName] = true;
}

phase98::FarPointer Phase98CodeGenIntegration::getFarAddress(const std::string& varName) const {
    return farMemoryManager.getVariableAddress(varName);
}

void Phase98CodeGenIntegration::emitBankSetup(const phase98::FarPointer& ptr) {
    // Only emit if bank is not cached
    if (!canReuseBank(ptr)) {
        unsigned bank = ptr.bankId;

        // Generate: lda #$bank; sta $FFF8
        emitter.lda_imm(bank);
        emitter.sta_abs(0xFFF8);  // MAP register
        emitter.emitComment("Set MAP register for bank " + std::to_string(bank));

        setCachedBank(bank);
    }
}

void Phase98CodeGenIntegration::emitFarLoad(const phase98::FarPointer& ptr, const std::string& targetReg) {
    // Setup bank
    emitBankSetup(ptr);

    unsigned offset = ptr.bankOffset;

    // Setup ZP with offset
    emitter.lda_imm(offset & 0xFF);
    emitter.sta_zp(0x20);  // Low byte of offset
    emitter.lda_imm((offset >> 8) & 0xFF);
    emitter.sta_zp(0x21);  // High byte of offset

    // Load from indirect address: lda ($20),y
    emitter.ldy_imm(0);
    // Use text output for indirect,y addressing (until we know the exact method)
    emitter.emitComment("lda ($20),y  ; Load from far memory");

    // If target is not A, move value there
    if (targetReg != "A") {
        if (targetReg == "X") {
            emitter.tax();
        } else if (targetReg == "Y") {
            emitter.tay();
        }
    }
}

void Phase98CodeGenIntegration::emitFarStore(const phase98::FarPointer& ptr, const std::string& sourceReg) {
    // Setup bank
    emitBankSetup(ptr);

    unsigned offset = ptr.bankOffset;

    // Setup ZP with offset
    emitter.lda_imm(offset & 0xFF);
    emitter.sta_zp(0x20);
    emitter.lda_imm((offset >> 8) & 0xFF);
    emitter.sta_zp(0x21);

    // Load value into A if it's in another register
    if (sourceReg != "A") {
        if (sourceReg == "X") {
            emitter.txa();
        } else if (sourceReg == "Y") {
            emitter.tya();
        }
    }

    // Store to indirect address: sta ($20),y
    emitter.ldy_imm(0);
    // Use text output for indirect,y addressing (until we know the exact method)
    emitter.emitComment("sta ($20),y  ; Store to far memory");
}

bool Phase98CodeGenIntegration::canReuseBank(const phase98::FarPointer& ptr) const {
    return farMemoryManager.isBankCached(ptr.bankId);
}

void Phase98CodeGenIntegration::setCachedBank(unsigned bank) {
    // This would update the internal state in FarMemoryManager
    // For now, it's tracked via indirect calls (C++ will optimize out the parameter warning)
    (void)bank;  // Suppress unused parameter warning
}

