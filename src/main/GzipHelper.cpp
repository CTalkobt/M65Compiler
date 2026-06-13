#include "GzipHelper.hpp"
#include <zlib.h>
#include <algorithm>
#include <cstring>

namespace gzip {

std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed) {
    if (!isGzipped(compressed)) return {};

    z_stream strm = {};
    // 15 + 16 = gzip decoding mode
    if (inflateInit2(&strm, 15 + 16) != Z_OK) return {};

    strm.avail_in = (uInt)compressed.size();
    strm.next_in = const_cast<Bytef*>(compressed.data());

    std::vector<uint8_t> result;
    uint8_t buf[16384];
    int ret;
    do {
        strm.avail_out = sizeof(buf);
        strm.next_out = buf;
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            return {};
        }
        size_t have = sizeof(buf) - strm.avail_out;
        result.insert(result.end(), buf, buf + have);
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return result;
}

std::vector<uint8_t> compress(const std::vector<uint8_t>& data) {
    z_stream strm = {};
    // 15 + 16 = gzip encoding mode
    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                     15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return {};

    strm.avail_in = (uInt)data.size();
    strm.next_in = const_cast<Bytef*>(data.data());

    std::vector<uint8_t> result;
    uint8_t buf[16384];
    int ret;
    do {
        strm.avail_out = sizeof(buf);
        strm.next_out = buf;
        ret = deflate(&strm, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&strm);
            return {};
        }
        size_t have = sizeof(buf) - strm.avail_out;
        result.insert(result.end(), buf, buf + have);
    } while (strm.avail_out == 0);

    deflateEnd(&strm);
    return result;
}

bool hasGzExtension(const std::string& path) {
    if (path.size() < 3) return false;
    std::string ext = path.substr(path.size() - 3);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".gz";
}

std::string stripGzExtension(const std::string& path) {
    if (hasGzExtension(path)) return path.substr(0, path.size() - 3);
    return path;
}

} // namespace gzip
