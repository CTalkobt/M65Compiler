// ZipArchive — reading and rewriting .zip files (PKWARE APPNOTE: local
// headers, central directory, end record; methods 0 (stored) and 8
// (deflate, through zlib)). Used by disk45 for disk images inside a zip.
// ZIP64, encryption and multi-disk archives are not supported.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

class ZipArchive {
public:
    struct Member {
        std::string name;
        uint16_t method = 0;       // 0 stored, 8 deflate
        uint16_t time = 0, date = 0;  // MS-DOS
        uint32_t crc = 0;
        uint32_t compSize = 0, size = 0;
        uint32_t extAttr = 0;
        std::vector<uint8_t> compressed;  // as stored in the archive
    };

    bool load(const std::string& path, std::string& err);
    bool save(const std::string& path, std::string& err) const;

    const std::vector<Member>& members() const { return members_; }
    int find(const std::string& name) const;   // case-insensitive; -1 if none
    bool extract(int index, std::vector<uint8_t>& data, std::string& err) const;
    // Add or replace a member (deflated)
    void put(const std::string& name, const std::vector<uint8_t>& data);

    static bool isZip(const std::vector<uint8_t>& data);

private:
    std::vector<Member> members_;
};
