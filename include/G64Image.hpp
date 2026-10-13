#pragma once
#include "D64Image.hpp"
#include "D71Image.hpp"
#include "GcrTracks.hpp"

// G64: GCR-encoded 1541 disk image; G71: the 1571 (double-sided) form
// (Schepers' G64.TXT; layout in GcrTracks.hpp).
//
// The tracks are decoded into D64/D71 sectors on load (Gcr::decodeTrack:
// bit-level syncs, header and data checksums). Saving writes a G64/G71 in
// which unchanged tracks keep their original GCR and changed ones are
// encoded as standard tracks; create makes an image of standard tracks.

class G64Image : public D64Image {
public:
    bool loadFromFile(const std::string& path) override;
    bool saveToFile(const std::string& path) const override;
    void format(const std::string& diskName = "", const std::string& diskId = "CC") override;
    static bool isG64File(const std::vector<uint8_t>& data);
    int sectorsDecoded() const { return tracks_.sectorsDecoded(); }

protected:
    GcrTracks tracks_;
};

class G71Image : public D71Image {
public:
    bool loadFromFile(const std::string& path) override;
    bool saveToFile(const std::string& path) const override;
    void format(const std::string& diskName = "", const std::string& diskId = "CC") override;
    int sectorsDecoded() const { return tracks_.sectorsDecoded(); }

private:
    GcrTracks tracks_;
};
