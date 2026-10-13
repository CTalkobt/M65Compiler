// O45WriterPhases.cpp — phase 4.2's IPO hints option (OPT_IPO_HINTS): the
// serialization of O45IPOHints. Nothing sets IPO hints, so O45Writer no longer
// writes the option; this is built into lib45-phases.a, which no tool links.

#include "O45Writer.hpp"
#include "O45IRSerializer.hpp"
#include <stdexcept>

// Helper function to serialize O45IPOHints to binary format
std::vector<uint8_t> serializeIPOHints(const O45IPOHints& hints) {
    std::vector<uint8_t> data;

    // Version byte
    data.push_back(hints.version);

    // Function count (2 bytes LE)
    uint16_t funcCount = (uint16_t)hints.functions.size();
    data.push_back((uint8_t)(funcCount & 0xFF));
    data.push_back((uint8_t)(funcCount >> 8));

    // Serialize each function's hints
    for (const auto& func : hints.functions) {
        // Function name (NUL-terminated string)
        for (char c : func.functionName) data.push_back((uint8_t)c);
        data.push_back(0x00);

        // Call count (2 bytes LE)
        data.push_back((uint8_t)(func.callCount & 0xFF));
        data.push_back((uint8_t)(func.callCount >> 8));

        // Estimated code size (2 bytes LE)
        data.push_back((uint8_t)(func.estimatedCodeSize & 0xFF));
        data.push_back((uint8_t)(func.estimatedCodeSize >> 8));

        // Flags (1 byte)
        data.push_back(func.flags);

        // External call count (1 byte)
        data.push_back(func.externalCallCount);

        // Specialization pattern count (1 byte, max 255)
        uint8_t specCount = (uint8_t)std::min(size_t(255), func.specializations.size());
        data.push_back(specCount);

        // Serialize each specialization pattern
        for (size_t i = 0; i < specCount; i++) {
            const auto& spec = func.specializations[i];

            // Argument count (1 byte, max 255)
            uint8_t argCount = (uint8_t)std::min(size_t(255), spec.argumentValues.size());
            data.push_back(argCount);

            // Argument values (8 bytes LE each for int64_t)
            for (size_t j = 0; j < argCount; j++) {
                int64_t val = spec.argumentValues[j];
                for (int shift = 0; shift < 8; shift++) {
                    data.push_back((uint8_t)((val >> (shift * 8)) & 0xFF));
                }
            }

            // Frequency percentage (1 byte, 0-100)
            data.push_back(spec.frequency);
        }
    }

    return data;
}
