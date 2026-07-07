#include "BAMOperations.hpp"

// ============================================================================
// D64 BAM Operations Implementation
// ============================================================================

bool D64BAMOperations::isSectorFree(const uint8_t* bamData, int track, int sector) const {
    if (track < 1 || track > 35) return false;
    if (sector < 0 || sector >= 24) return false; // Max sectors per track is 21

    int off = bamOffsetForTrack(track);
    int byteIdx = 1 + (sector / 8);
    int bitIdx = sector % 8;
    return (bamData[off + byteIdx] >> bitIdx) & 1;
}

bool D64BAMOperations::allocateSector(uint8_t* bamData, int track, int sector) {
    if (track < 1 || track > 35) return false;
    if (sector < 0 || sector >= 24) return false;

    int off = bamOffsetForTrack(track);
    int byteIdx = 1 + (sector / 8);
    int bitIdx = sector % 8;

    // Check if free (bit = 1)
    if (!((bamData[off + byteIdx] >> bitIdx) & 1)) return false;

    // Clear bit (0 = allocated)
    bamData[off + byteIdx] &= ~(1 << bitIdx);

    // Decrement free count (stored in byte 0 of this track's BAM entry)
    bamData[off]--;

    return true;
}

bool D64BAMOperations::freeSector(uint8_t* bamData, int track, int sector) {
    if (track < 1 || track > 35) return false;
    if (sector < 0 || sector >= 24) return false;

    int off = bamOffsetForTrack(track);
    int byteIdx = 1 + (sector / 8);
    int bitIdx = sector % 8;

    // Check if already free (bit = 1)
    if ((bamData[off + byteIdx] >> bitIdx) & 1) return false;

    // Set bit (1 = free)
    bamData[off + byteIdx] |= (1 << bitIdx);

    // Increment free count
    bamData[off]++;

    return true;
}

int D64BAMOperations::countFreeSectors(const uint8_t* bamData) const {
    int count = 0;
    for (int track = 1; track <= 35; track++) {
        int off = bamOffsetForTrack(track);
        count += bamData[off]; // free count for this track
    }
    return count;
}

// ============================================================================
// D81 BAM Operations Implementation
// ============================================================================

bool D81BAMOperations::isSectorFree(const uint8_t* bamData, int track, int sector) const {
    if (track < 1 || track > 80 || sector < 0 || sector >= 40) return false;

    // Normalize track to 0-39 range within this BAM sector
    int trackLocal = (track <= 40) ? (track - 1) : (track - 41);

    int byteOff, bitOff;
    bamBitPos(trackLocal, sector, byteOff, bitOff);

    return (bamData[byteOff] >> bitOff) & 1;
}

bool D81BAMOperations::allocateSector(uint8_t* bamData, int track, int sector) {
    if (track < 1 || track > 80 || sector < 0 || sector >= 40) return false;

    int trackLocal = (track <= 40) ? (track - 1) : (track - 41);

    int byteOff, bitOff;
    bamBitPos(trackLocal, sector, byteOff, bitOff);

    // Check if free (bit = 1)
    if (!((bamData[byteOff] >> bitOff) & 1)) return false;

    // Clear bit (0 = allocated)
    bamData[byteOff] &= ~(1 << bitOff);

    // Decrement free count
    bamData[freeCountOffset(trackLocal)]--;

    return true;
}

bool D81BAMOperations::freeSector(uint8_t* bamData, int track, int sector) {
    if (track < 1 || track > 80 || sector < 0 || sector >= 40) return false;

    int trackLocal = (track <= 40) ? (track - 1) : (track - 41);

    int byteOff, bitOff;
    bamBitPos(trackLocal, sector, byteOff, bitOff);

    // Check if already free (bit = 1)
    if ((bamData[byteOff] >> bitOff) & 1) return false;

    // Set bit (1 = free)
    bamData[byteOff] |= (1 << bitOff);

    // Increment free count
    bamData[freeCountOffset(trackLocal)]++;

    return true;
}

int D81BAMOperations::countFreeSectors(const uint8_t* bamData) const {
    // For D81, the caller is responsible for calling this for each BAM sector
    // and summing the results, since each BAM sector covers different tracks
    int count = 0;
    for (int trackLocal = 0; trackLocal < 40; trackLocal++) {
        count += bamData[freeCountOffset(trackLocal)];
    }
    return count;
}
