// disk45_catalog.cpp — Searchable catalog for CBM disk/tape image collections
//
// Uses SQLite3 to index files across large collections of disk images.
// Schema:
//   images(id, path, format, name, disk_id, mtime, size)
//   files(id, image_id, name, type, size_bytes, size_sectors, crc32)

#include "DiskImage.hpp"
#include <sqlite3.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

// ============================================================================
// CRC32 (simple implementation for file content hashing)
// ============================================================================

static uint32_t crc32_table[256];
static bool crc32_init = false;

static void init_crc32() {
    if (crc32_init) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc32_init = true;
}

static uint32_t compute_crc32(const std::vector<uint8_t>& data) {
    init_crc32();
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t b : data) crc = crc32_table[(crc ^ b) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

// ============================================================================
// Database helpers
// ============================================================================

static bool db_exec(sqlite3* db, const char* sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "SQLite error: " << (err ? err : "unknown") << "\n";
        sqlite3_free(err);
        return false;
    }
    return true;
}

static bool create_schema(sqlite3* db) {
    return db_exec(db,
        "CREATE TABLE IF NOT EXISTS images ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  path TEXT UNIQUE NOT NULL,"
        "  format TEXT,"
        "  name TEXT,"
        "  disk_id TEXT,"
        "  mtime INTEGER,"
        "  size INTEGER"
        ");"
        "CREATE TABLE IF NOT EXISTS files ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  image_id INTEGER NOT NULL,"
        "  name TEXT NOT NULL,"
        "  type TEXT NOT NULL,"
        "  size_bytes INTEGER,"
        "  size_sectors INTEGER,"
        "  crc32 TEXT,"
        "  FOREIGN KEY(image_id) REFERENCES images(id) ON DELETE CASCADE"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_files_name ON files(name);"
        "CREATE INDEX IF NOT EXISTS idx_files_image ON files(image_id);"
        "CREATE INDEX IF NOT EXISTS idx_files_crc ON files(crc32);"
        "CREATE INDEX IF NOT EXISTS idx_images_path ON images(path);"
    );
}

static const char* fileTypeStr(CbmFileType t) {
    switch (t) {
        case CbmFileType::DEL: return "DEL";
        case CbmFileType::SEQ: return "SEQ";
        case CbmFileType::PRG: return "PRG";
        case CbmFileType::USR: return "USR";
        case CbmFileType::REL: return "REL";
        case CbmFileType::CBM: return "CBM";
    }
    return "???";
}

static std::string formatFromPath(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return "???";
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    // Strip .gz
    if (ext == ".gz") {
        auto dot2 = path.rfind('.', dot - 1);
        if (dot2 != std::string::npos) ext = path.substr(dot2, dot - dot2) + ".gz";
    }
    return ext;
}

// ============================================================================
// Scan an image and index its files
// ============================================================================

static bool indexImage(sqlite3* db, const std::string& path, bool verbose) {
    // Check if already indexed with same mtime
    auto mtime = fs::last_write_time(path);
    auto mtimeVal = mtime.time_since_epoch().count();
    auto fileSize = (int64_t)fs::file_size(path);

    sqlite3_stmt* chk;
    sqlite3_prepare_v2(db, "SELECT id, mtime FROM images WHERE path = ?", -1, &chk, nullptr);
    sqlite3_bind_text(chk, 1, path.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(chk) == SQLITE_ROW) {
        int64_t existingMtime = sqlite3_column_int64(chk, 1);
        if (existingMtime == mtimeVal) {
            sqlite3_finalize(chk);
            return true; // already up to date
        }
        // Mtime changed — delete old entries and re-index
        int64_t imgId = sqlite3_column_int64(chk, 0);
        sqlite3_finalize(chk);
        sqlite3_stmt* del;
        sqlite3_prepare_v2(db, "DELETE FROM files WHERE image_id = ?", -1, &del, nullptr);
        sqlite3_bind_int64(del, 1, imgId);
        sqlite3_step(del);
        sqlite3_finalize(del);
        sqlite3_prepare_v2(db, "DELETE FROM images WHERE id = ?", -1, &del, nullptr);
        sqlite3_bind_int64(del, 1, imgId);
        sqlite3_step(del);
        sqlite3_finalize(del);
    } else {
        sqlite3_finalize(chk);
    }

    auto img = DiskImage::load(path);
    if (!img) return false;

    std::string format = formatFromPath(path);
    std::string diskName = img->diskName();
    std::string diskId = img->diskId();

    // Insert image
    sqlite3_stmt* ins;
    sqlite3_prepare_v2(db,
        "INSERT INTO images (path, format, name, disk_id, mtime, size) VALUES (?,?,?,?,?,?)",
        -1, &ins, nullptr);
    sqlite3_bind_text(ins, 1, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(ins, 2, format.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(ins, 3, diskName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(ins, 4, diskId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(ins, 5, mtimeVal);
    sqlite3_bind_int64(ins, 6, fileSize);
    sqlite3_step(ins);
    sqlite3_finalize(ins);

    int64_t imgId = sqlite3_last_insert_rowid(db);

    // Index files
    auto files = img->listFiles();
    sqlite3_prepare_v2(db,
        "INSERT INTO files (image_id, name, type, size_bytes, size_sectors, crc32) VALUES (?,?,?,?,?,?)",
        -1, &ins, nullptr);

    int count = 0;
    for (auto& f : files) {
        // Compute CRC32 of file content
        auto data = img->readFile(f.name);
        uint32_t crc = data.empty() ? 0 : compute_crc32(data);
        char crcStr[16];
        snprintf(crcStr, sizeof(crcStr), "%08X", crc);

        sqlite3_bind_int64(ins, 1, imgId);
        sqlite3_bind_text(ins, 2, f.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(ins, 3, fileTypeStr(f.type), -1, SQLITE_STATIC);
        sqlite3_bind_int(ins, 4, f.sizeInBytes);
        sqlite3_bind_int(ins, 5, f.sizeInSectors);
        sqlite3_bind_text(ins, 6, crcStr, -1, SQLITE_STATIC);
        sqlite3_step(ins);
        sqlite3_reset(ins);
        count++;
    }
    sqlite3_finalize(ins);

    if (verbose) std::cout << "  " << path << " (" << count << " files)\n";
    return true;
}

// ============================================================================
// Catalog commands
// ============================================================================

static std::string defaultDbPath() {
    // Default: ~/.disk45-catalog.db
    const char* home = getenv("HOME");
    if (home) return std::string(home) + "/.disk45-catalog.db";
    return "disk45-catalog.db";
}

static std::string getDbPath(int& argc, char**& argv) {
    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "--db" && i + 1 < argc) {
            std::string path = argv[i + 1];
            // Remove --db and its arg
            for (int j = i; j < argc - 2; j++) argv[j] = argv[j + 2];
            argc -= 2;
            return path;
        }
    }
    return defaultDbPath();
}

// Supported image extensions for scanning
static bool isSupportedImage(const std::string& path) {
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    // Strip .gz
    if (lower.size() > 3 && lower.substr(lower.size() - 3) == ".gz")
        lower = lower.substr(0, lower.size() - 3);
    auto dot = lower.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = lower.substr(dot);
    return ext == ".d64" || ext == ".d71" || ext == ".d81" || ext == ".d65" ||
           ext == ".d80" || ext == ".d82" || ext == ".ark" || ext == ".arc" ||
           ext == ".sda" || ext == ".lnx" || ext == ".tap" || ext == ".t64" ||
           ext == ".g64" || ext == ".g71" || ext == ".cvt";
}

int disk45_catalog(int argc, char** argv) {
    if (argc < 1) {
        std::cerr << "Usage: disk45 catalog <command> [options]\n"
                  << "Commands:\n"
                  << "  build <dir> [--db file]      Scan directory, index all images\n"
                  << "  update <dir> [--db file]     Incremental update (skip unchanged)\n"
                  << "  search <pattern> [--db file]  Search files by name (* and ? wildcards)\n"
                  << "  list [--db file]             Summary of indexed images\n"
                  << "  duplicates [--db file]        Find duplicate files by CRC32\n"
                  << "  stats [--db file]            Collection statistics\n";
        return 1;
    }

    std::string subcmd = argv[0];
    argc--; argv++;

    std::string dbPath = getDbPath(argc, argv);

    sqlite3* db;
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: cannot open catalog database: " << dbPath << "\n";
        return 1;
    }
    create_schema(db);
    db_exec(db, "PRAGMA journal_mode=WAL;");
    db_exec(db, "PRAGMA synchronous=NORMAL;");

    if (subcmd == "build" || subcmd == "update") {
        if (argc < 1) {
            std::cerr << "Usage: disk45 catalog " << subcmd << " <directory> [--db file]\n";
            sqlite3_close(db);
            return 1;
        }
        std::string dir = argv[0];
        int indexed = 0, skipped = 0, errors = 0;

        db_exec(db, "BEGIN TRANSACTION;");

        std::cout << "Scanning " << dir << " ...\n";
        for (auto& entry : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied)) {
            if (!entry.is_regular_file()) continue;
            std::string path = entry.path().string();
            if (!isSupportedImage(path)) continue;

            try {
                if (indexImage(db, path, true)) indexed++;
                else errors++;
            } catch (...) {
                errors++;
            }
        }

        db_exec(db, "COMMIT;");
        std::cout << "Indexed " << indexed << " image(s)";
        if (errors) std::cout << ", " << errors << " error(s)";
        std::cout << " → " << dbPath << "\n";

    } else if (subcmd == "search") {
        if (argc < 1) {
            std::cerr << "Usage: disk45 catalog search <pattern> [--db file]\n";
            sqlite3_close(db);
            return 1;
        }
        // Convert glob to SQL LIKE
        std::string pattern = argv[0];
        std::string sqlPattern;
        for (char c : pattern) {
            if (c == '*') sqlPattern += '%';
            else if (c == '?') sqlPattern += '_';
            else sqlPattern += c;
        }
        // Case-insensitive
        std::transform(sqlPattern.begin(), sqlPattern.end(), sqlPattern.begin(), ::toupper);

        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db,
            "SELECT f.name, f.type, f.size_sectors, f.crc32, i.path, i.name AS disk_name "
            "FROM files f JOIN images i ON f.image_id = i.id "
            "WHERE UPPER(f.name) LIKE ? "
            "ORDER BY f.name, i.path",
            -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, sqlPattern.c_str(), -1, SQLITE_STATIC);

        int count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* fname = (const char*)sqlite3_column_text(stmt, 0);
            const char* ftype = (const char*)sqlite3_column_text(stmt, 1);
            int sectors = sqlite3_column_int(stmt, 2);
            const char* crc = (const char*)sqlite3_column_text(stmt, 3);
            const char* ipath = (const char*)sqlite3_column_text(stmt, 4);
            const char* dname = (const char*)sqlite3_column_text(stmt, 5);

            std::cout << "  " << std::left << std::setw(18) << (fname ? fname : "")
                      << std::setw(5) << (ftype ? ftype : "")
                      << std::right << std::setw(5) << sectors << " blk  "
                      << (crc ? crc : "") << "  "
                      << (ipath ? ipath : "") << "\n";
            count++;
        }
        sqlite3_finalize(stmt);
        std::cout << count << " result(s)\n";

    } else if (subcmd == "list") {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db,
            "SELECT i.path, i.format, i.name, "
            "  (SELECT COUNT(*) FROM files WHERE image_id = i.id) AS file_count "
            "FROM images i ORDER BY i.path",
            -1, &stmt, nullptr);

        int count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* path = (const char*)sqlite3_column_text(stmt, 0);
            const char* fmt = (const char*)sqlite3_column_text(stmt, 1);
            const char* name = (const char*)sqlite3_column_text(stmt, 2);
            int files = sqlite3_column_int(stmt, 3);

            std::cout << "  " << std::left << std::setw(40) << (path ? path : "")
                      << std::setw(8) << (fmt ? fmt : "")
                      << std::setw(18) << (name ? name : "")
                      << files << " files\n";
            count++;
        }
        sqlite3_finalize(stmt);
        std::cout << count << " image(s) in catalog\n";

    } else if (subcmd == "duplicates") {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db,
            "SELECT f.name, f.type, f.crc32, f.size_sectors, i.path "
            "FROM files f JOIN images i ON f.image_id = i.id "
            "WHERE f.crc32 IN ("
            "  SELECT crc32 FROM files WHERE crc32 != '00000000' "
            "  GROUP BY crc32 HAVING COUNT(*) > 1"
            ") ORDER BY f.crc32, f.name, i.path",
            -1, &stmt, nullptr);

        std::string lastCrc;
        int groups = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* fname = (const char*)sqlite3_column_text(stmt, 0);
            const char* ftype = (const char*)sqlite3_column_text(stmt, 1);
            const char* crc = (const char*)sqlite3_column_text(stmt, 2);
            int sectors = sqlite3_column_int(stmt, 3);
            const char* ipath = (const char*)sqlite3_column_text(stmt, 4);

            std::string crcStr = crc ? crc : "";
            if (crcStr != lastCrc) {
                if (!lastCrc.empty()) std::cout << "\n";
                std::cout << "CRC " << crcStr << ":\n";
                lastCrc = crcStr;
                groups++;
            }
            std::cout << "  " << std::left << std::setw(18) << (fname ? fname : "")
                      << std::setw(5) << (ftype ? ftype : "")
                      << std::right << std::setw(5) << sectors << " blk  "
                      << (ipath ? ipath : "") << "\n";
        }
        sqlite3_finalize(stmt);
        std::cout << "\n" << groups << " duplicate group(s)\n";

    } else if (subcmd == "stats") {
        sqlite3_stmt* stmt;
        int images = 0, files = 0;
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM images", -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) images = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM files", -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) files = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        std::cout << "Catalog: " << dbPath << "\n"
                  << "  Images: " << images << "\n"
                  << "  Files:  " << files << "\n";

        // Format breakdown
        sqlite3_prepare_v2(db,
            "SELECT format, COUNT(*) FROM images GROUP BY format ORDER BY COUNT(*) DESC",
            -1, &stmt, nullptr);
        std::cout << "  Formats:\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* fmt = (const char*)sqlite3_column_text(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            std::cout << "    " << std::left << std::setw(10) << (fmt ? fmt : "?")
                      << cnt << "\n";
        }
        sqlite3_finalize(stmt);

        // File type breakdown
        sqlite3_prepare_v2(db,
            "SELECT type, COUNT(*) FROM files GROUP BY type ORDER BY COUNT(*) DESC",
            -1, &stmt, nullptr);
        std::cout << "  File types:\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* t = (const char*)sqlite3_column_text(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            std::cout << "    " << std::left << std::setw(10) << (t ? t : "?")
                      << cnt << "\n";
        }
        sqlite3_finalize(stmt);

    } else {
        std::cerr << "Unknown catalog command: " << subcmd << "\n";
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);
    return 0;
}
