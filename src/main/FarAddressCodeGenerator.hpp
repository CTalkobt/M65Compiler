// Phase 98.3: Code Generation Integration
// Hooks for emitting far memory operations in CodeGenerator

#pragma once

#include "FarAddressMemorySupport.hpp"
#include "M65Emitter.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>

// Integration layer between CodeGenerator and FarAddressMemorySupport
class FarAddressCodeGenerator {
public:
    explicit FarAddressCodeGenerator(M65Emitter& emitter);

    // Detect if variable uses far addressing
    bool isFarVariable(const std::string& varName) const;

    // Allocate far memory for a global variable
    void allocateFarVariable(const std::string& varName, size_t size);

    // Get far address for a variable
    phase98::FarPointer getFarAddress(const std::string& varName) const;

    // Emit bank setup for accessing far variable
    void emitBankSetup(const phase98::FarPointer& ptr);

    // Emit far memory load
    void emitFarLoad(const phase98::FarPointer& ptr, const std::string& targetReg = "A");

    // Emit far memory store
    void emitFarStore(const phase98::FarPointer& ptr, const std::string& sourceReg = "A");

    // Check if current bank matches target (optimization)
    bool canReuseBank(const phase98::FarPointer& ptr) const;

    // Update cached bank state
    void setCachedBank(unsigned bank);

    // Statistics
    int getFarVariableCount() const { return farMemoryManager.getBankCount(); }
    size_t getTotalFarMemoryUsed() const { return farMemoryManager.getTotalFarMemoryUsed(); }

private:
    M65Emitter& emitter;
    phase98::FarMemoryManager farMemoryManager;

    // Track variables designated as far
    std::map<std::string, bool> farVariables;
};

