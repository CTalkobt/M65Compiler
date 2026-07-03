// disk45_fuse.cpp — FUSE3 filesystem mount for CBM disk images
//
// Usage: disk45 mount <image> <mountpoint> [fuse_options]
//        fusermount -u <mountpoint>
//
// Maps CBM files to POSIX filenames with type extensions (.prg, .seq, etc.)
// Read/write support for sector-based images. Read-only for TAP/G64/CVT.

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <algorithm>

#include "DiskImage.hpp"

// ============================================================================
// Global state (FUSE callbacks use global context)
// ============================================================================

static std::unique_ptr<DiskImage> g_img;
static std::string g_imagePath;
static std::mutex g_mutex;
static bool g_readonly = false;

// Map POSIX filename → CBM name and type
struct CbmEntry {
    std::string cbmName;
    CbmFileType type;
    uint32_t size;
};

static std::string typeExt(CbmFileType t) {
    switch (t) {
        case CbmFileType::PRG: return ".prg";
        case CbmFileType::SEQ: return ".seq";
        case CbmFileType::USR: return ".usr";
        case CbmFileType::REL: return ".rel";
        case CbmFileType::CBM: return "";  // directories have no extension
        default: return ".del";
    }
}

static CbmFileType extToType(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return CbmFileType::PRG;
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".prg") return CbmFileType::PRG;
    if (ext == ".seq") return CbmFileType::SEQ;
    if (ext == ".usr") return CbmFileType::USR;
    if (ext == ".rel") return CbmFileType::REL;
    return CbmFileType::PRG;
}

// Strip leading / and type extension to get CBM name
static std::string pathToCbmName(const char* path) {
    std::string p = path;
    if (!p.empty() && p[0] == '/') p = p.substr(1);
    // Strip extension
    auto dot = p.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = p.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".prg" || ext == ".seq" || ext == ".usr" || ext == ".rel" || ext == ".del")
            p = p.substr(0, dot);
    }
    // Uppercase for CBM
    std::transform(p.begin(), p.end(), p.begin(), ::toupper);
    return p;
}

static std::string cbmToPath(const FileInfo& f) {
    std::string name = f.name;
    // Sanitize for POSIX
    for (auto& c : name) {
        if (c == '/' || c == '\\' || c == ':' || c < 32) c = '_';
    }
    return name + typeExt(f.type);
}

// ============================================================================
// FUSE callbacks
// ============================================================================

static int cbm_getattr(const char* path, struct stat* st, struct fuse_file_info*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    memset(st, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    std::string cbmName = pathToCbmName(path);
    auto files = g_img->listFiles();
    for (auto& f : files) {
        if (f.name == cbmName) {
            if (f.type == CbmFileType::CBM) {
                st->st_mode = S_IFDIR | 0755;
                st->st_nlink = 2;
            } else {
                st->st_mode = S_IFREG | (g_readonly ? 0444 : 0644);
                st->st_nlink = 1;
                // Get actual file size
                auto data = g_img->readFile(f.name);
                st->st_size = data.size();
            }
            return 0;
        }
    }
    return -ENOENT;
}

static int cbm_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                        off_t, struct fuse_file_info*, enum fuse_readdir_flags) {
    std::lock_guard<std::mutex> lock(g_mutex);

    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);

    if (strcmp(path, "/") != 0) return -ENOENT; // subdirs not yet supported in FUSE

    auto files = g_img->listFiles();
    for (auto& f : files) {
        std::string posixName = cbmToPath(f);
        filler(buf, posixName.c_str(), NULL, 0, (fuse_fill_dir_flags)0);
    }
    return 0;
}

static int cbm_open(const char* path, struct fuse_file_info* fi) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string cbmName = pathToCbmName(path);
    if (!g_img->fileExists(cbmName)) return -ENOENT;
    if (g_readonly && (fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
    return 0;
}

static int cbm_read(const char* path, char* buf, size_t size, off_t offset,
                     struct fuse_file_info*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string cbmName = pathToCbmName(path);
    auto data = g_img->readFile(cbmName);
    if (data.empty() && !g_img->fileExists(cbmName)) return -ENOENT;

    if (offset >= (off_t)data.size()) return 0;
    size_t avail = data.size() - offset;
    if (size > avail) size = avail;
    memcpy(buf, data.data() + offset, size);
    return (int)size;
}

static int cbm_write(const char* path, const char* buf, size_t size, off_t offset,
                      struct fuse_file_info*) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string cbmName = pathToCbmName(path);

    // Read existing file, modify, rewrite
    auto data = g_img->readFile(cbmName);

    // Extend if needed
    size_t needed = offset + size;
    if (needed > data.size()) data.resize(needed, 0);
    memcpy(data.data() + offset, buf, size);

    // Remove old and re-add
    CbmFileType ftype = extToType(path);
    g_img->removeFile(cbmName);
    if (!g_img->addFile(cbmName, ftype, data)) return -ENOSPC;
    g_img->saveToFile(g_imagePath);
    return (int)size;
}

static int cbm_create(const char* path, mode_t, struct fuse_file_info*) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string cbmName = pathToCbmName(path);
    if (cbmName.size() > 16) return -ENAMETOOLONG;

    CbmFileType ftype = extToType(path);
    std::vector<uint8_t> empty;
    if (!g_img->addFile(cbmName, ftype, empty)) return -ENOSPC;
    g_img->saveToFile(g_imagePath);
    return 0;
}

static int cbm_unlink(const char* path) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string cbmName = pathToCbmName(path);
    if (!g_img->removeFile(cbmName)) return -ENOENT;
    g_img->saveToFile(g_imagePath);
    return 0;
}

static int cbm_rename(const char* from, const char* to, unsigned int) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string oldName = pathToCbmName(from);
    std::string newName = pathToCbmName(to);
    if (newName.size() > 16) return -ENAMETOOLONG;
    if (!g_img->renameFile(oldName, newName)) return -ENOENT;
    g_img->saveToFile(g_imagePath);
    return 0;
}

static int cbm_truncate(const char* path, off_t size, struct fuse_file_info*) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string cbmName = pathToCbmName(path);
    auto data = g_img->readFile(cbmName);
    if (data.empty() && !g_img->fileExists(cbmName)) return -ENOENT;
    data.resize(size, 0);
    CbmFileType ftype = extToType(path);
    g_img->removeFile(cbmName);
    if (!g_img->addFile(cbmName, ftype, data)) return -ENOSPC;
    g_img->saveToFile(g_imagePath);
    return 0;
}

static int cbm_utimens(const char*, const struct timespec*, struct fuse_file_info*) {
    return 0; // CBM has no timestamps (ignore)
}

static int cbm_statfs(const char*, struct statvfs* st) {
    std::lock_guard<std::mutex> lock(g_mutex);
    memset(st, 0, sizeof(struct statvfs));
    st->f_bsize = 254;
    st->f_frsize = 254;
    st->f_blocks = g_img->totalSectors();
    st->f_bfree = g_img->freeSectors();
    st->f_bavail = g_img->freeSectors();
    st->f_namemax = 16;
    return 0;
}

// ============================================================================
// FUSE operations table
// ============================================================================

static struct fuse_operations cbm_ops;

static void init_cbm_ops() {
    memset(&cbm_ops, 0, sizeof(cbm_ops));
    cbm_ops.getattr  = cbm_getattr;
    cbm_ops.unlink   = cbm_unlink;
    cbm_ops.rename   = cbm_rename;
    cbm_ops.truncate = cbm_truncate;
    cbm_ops.open     = cbm_open;
    cbm_ops.read     = cbm_read;
    cbm_ops.write    = cbm_write;
    cbm_ops.statfs   = cbm_statfs;
    cbm_ops.readdir  = cbm_readdir;
    cbm_ops.create   = cbm_create;
    cbm_ops.utimens  = cbm_utimens;
}

// ============================================================================
// Entry point (called from disk45 mount command)
// ============================================================================

int disk45_fuse_main(const std::string& imagePath, const std::string& mountpoint,
                      bool readonly, int argc, char** argv) {
    g_imagePath = imagePath;
    g_readonly = readonly;

    g_img = DiskImage::load(imagePath);
    if (!g_img) {
        fprintf(stderr, "Error: failed to load %s\n", imagePath.c_str());
        return 1;
    }

    // Detect read-only formats
    if (g_img->totalTracks() == 0) g_readonly = true;

    init_cbm_ops();

    printf("Mounting %s on %s (%s)\n", imagePath.c_str(), mountpoint.c_str(),
           g_readonly ? "read-only" : "read-write");

    // Build FUSE argv: progname, mountpoint, -f (foreground), -s (single-threaded)
    std::vector<const char*> fuse_argv;
    fuse_argv.push_back("disk45");
    fuse_argv.push_back(mountpoint.c_str());
    fuse_argv.push_back("-f"); // foreground
    fuse_argv.push_back("-s"); // single-threaded (we use a mutex but simpler this way)
    // Pass through any extra FUSE options
    for (int i = 0; i < argc; i++) fuse_argv.push_back(argv[i]);

    return fuse_main((int)fuse_argv.size(), (char**)fuse_argv.data(), &cbm_ops, NULL);
}
