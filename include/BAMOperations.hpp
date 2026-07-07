#pragma once

#include <cstdint>
#include "DiskImage.hpp"

// ============================================================================
// BAM (Block Allocation Map) Operations Helper
// Unifies sector allocation logic across multiple disk formats
// Supports different BAM layouts: single-sector, multi-sector, variable-geometry
// ============================================================================

class BAMOperations {
public:
    virtual ~BAMOperations() = default;

    // Query sector free/allocated status
    virtual bool isSectorFree(const uint8_t* bamData, int track, int sector) const = 0;

    // Allocate a sector (mark as used, decrement free count)
    // Returns true if successful, false if already allocated
    virtual bool allocateSector(uint8_t* bamData, int track, int sector) = 0;

    // Free a sector (mark as unused, increment free count)
    // Returns true if successful, false if already free
    virtual bool freeSector(uint8_t* bamData, int track, int sector) = 0;

    // Count total free sectors
    virtual int countFreeSectors(const uint8_t* bamData) const = 0;
};

// ============================================================================
// D64 BAM Operations: Single BAM sector at track 18, sector 0
// ============================================================================

class D64BAMOperations : public BAMOperations {
public:
    // D64 BAM layout: bytes $04-$8B, 4 bytes per track
    static constexpr int BAM_BASE_OFFSET = 0x04;
    static constexpr int BYTES_PER_TRACK = 4;

    bool isSectorFree(const uint8_t* bamData, int track, int sector) const override;
    bool allocateSector(uint8_t* bamData, int track, int sector) override;
    bool freeSector(uint8_t* bamData, int track, int sector) override;
    int countFreeSectors(const uint8_t* bamData) const override;

private:
    // Get BAM offset (from start of BAM sector) for a track
    static int bamOffsetForTrack(int track) {
        return BAM_BASE_OFFSET + (track - 1) * BYTES_PER_TRACK;
    }
};

// ============================================================================
// D81 BAM Operations: Multi-sector BAM (tracks 1-40 in sector 1, 41-80 in sector 2)
// ============================================================================

class D81BAMOperations : public BAMOperations {
public:
    // D81 has two BAM sectors, each covering different track ranges
    // Tracks 1-40: BAM sector 1
    // Tracks 41-80: BAM sector 2
    // Each BAM sector has 40 tracks worth of allocation info starting at offset $10

    // Note: D81 uses dual BAM, so this class needs access to both sectors
    // The DiskImage base class provides sectorData() for this
    // For now, we assume the caller manages multiple BAM sectors

    bool isSectorFree(const uint8_t* bamData, int track, int sector) const override;
    bool allocateSector(uint8_t* bamData, int track, int sector) override;
    bool freeSector(uint8_t* bamData, int track, int sector) override;
    int countFreeSectors(const uint8_t* bamData) const override;

private:
    // D81 uses 6 bytes per track (1 byte free count + 5 bytes allocation bits)
    static constexpr int BYTES_PER_TRACK = 6;
    static constexpr int BAM_DATA_OFFSET = 0x10;

    // Get bit position within a BAM sector for a track/sector
    // Assumes track is already normalized to 0-39 range within this BAM sector
    void bamBitPos(int trackLocal, int sector, int& byteOff, int& bitOff) const {
        byteOff = BAM_DATA_OFFSET + trackLocal * BYTES_PER_TRACK + 1 + (sector / 8);
        bitOff = sector % 8;
    }

    // Get free count byte offset for a track (within the 40-track range of this BAM sector)
    int freeCountOffset(int trackLocal) const {
        return BAM_DATA_OFFSET + trackLocal * BYTES_PER_TRACK;
    }
};
