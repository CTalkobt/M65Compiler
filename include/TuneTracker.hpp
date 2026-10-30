// TuneTracker.hpp — tune45 import of an XM, S3M or IT: the file read into
// a MOD's structures (TuneMod.hpp), so the MOD's analysis and import take
// it as they are.
//
// The translation: the channels used, compacted (S3M: the enabled ones; IT:
// those with anything in a pattern; XM: all); the order list without S3M's
// and IT's "+++" and ending at "---"; the samples as one list (an XM's
// instruments' in turn; an IT's as numbered), 8-bit mono (16-bit: the high
// byte, rounded; stereo: the mean; a ping-pong loop unrolled); a note as
// the Amiga period it would have (XM's C-4, S3M's C-4 and IT's C-5 are
// ProTracker's C-2, period 428), the sample's tuning as its transpose; an
// instrument's note→sample map resolved into each cell; the effects as
// ProTracker's where there is one (the rest dropped).
#pragma once
#include "TuneMod.hpp"
#include <cstdint>
#include <string>
#include <vector>

// an XM ("Extended Module: " at 0), S3M ("SCRM" at 44) or IT ("IMPM" at 0) by its bytes
bool isTrackerModule(const std::vector<uint8_t>& file);
// read one into the MOD structures (TuneMod.hpp): false with error set
bool readTracker(const std::vector<uint8_t>& file, ModFile& m, std::string& error);
