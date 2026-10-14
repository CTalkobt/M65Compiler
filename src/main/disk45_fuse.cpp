// disk45_fuse.cpp — FUSE3 filesystem mount for CBM disk images
//
// Usage: disk45 mount <image> <mountpoint> [fuse_options]
//        fusermount -u <mountpoint>
//
// Maps CBM files to POSIX filenames with type extensions (.prg, .seq, etc.)
// Read/write support for sector-based images. Read-only for TAP/NIB/CVT.
// GEOS files are shown and written as their Convert (CVT) form, NAME.cvt.

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
#include <filesystem>

#include "DiskImage.hpp"
#include "GeosCvtImage.hpp"

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

// Strip leading / and type extension to get CBM name (for new files)
static std::string pathToCbmName(const char* path) {
    std::string p = path;
    p = p.substr(p.rfind('/') + 1);  // the last component
    // Strip extension
    auto dot = p.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = p.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".prg" || ext == ".seq" || ext == ".usr" || ext == ".rel" || ext == ".del" || ext == ".cvt")
            p = p.substr(0, dot);
    }
    // Uppercase for CBM
    std::transform(p.begin(), p.end(), p.begin(), ::toupper);
    return p;
}

static bool isCvtPath(const std::string& path) {
    if (path.size() < 4) return false;
    std::string ext = path.substr(path.size() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".cvt";
}

// GEOS files (on disk images) are shown as their Convert (CVT) form,
// "<name>.cvt": read like `disk45 extract`, and a .cvt written into the
// mount becomes a GEOS file when it is closed (as `disk45 add`)
static bool trackImage() {
    return g_img->totalTracks() > 0 && !dynamic_cast<GeosCvtImage*>(g_img.get());
}

static std::string cbmToPath(const FileInfo& f, bool geos) {
    std::string name = f.name;
    // Sanitize for POSIX
    for (auto& c : name) {
        if (c == '/' || c == '\\' || c == ':' || c < 32) c = '_';
    }
    return name + (geos ? std::string(".cvt") : typeExt(f.type));
}

// A file of the image as mounted
struct Node {
    FileInfo info;
    bool geos = false;
    DirEntry entry;
};

static bool isDir(const Node& n) { return n.info.type == CbmFileType::CBM && !n.geos; }

static std::vector<Node> nodes() {
    std::vector<Node> out;
    bool tracks = trackImage();
    for (auto& f : g_img->listFiles()) {
        Node n;
        n.info = f;
        if (tracks) {
            n.entry = g_img->findEntry(f.name);
            n.geos = n.entry.isValid() && g_img->isGeos(n.entry);
        }
        out.push_back(n);
    }
    return out;
}

// The entry of a listing a path component names: its listed name (exact),
// else ignoring case
static bool findIn(const std::vector<Node>& all, const std::string& p, Node& node) {
    for (auto& n : all)
        if (cbmToPath(n.info, n.geos) == p) { node = n; return true; }
    std::string lp = p;
    std::transform(lp.begin(), lp.end(), lp.begin(), ::tolower);
    for (auto& n : all) {
        std::string q = cbmToPath(n.info, n.geos);
        std::transform(q.begin(), q.end(), q.begin(), ::tolower);
        if (q == lp) { node = n; return true; }
    }
    return false;
}

static std::vector<std::string> pathParts(const char* path) {
    std::vector<std::string> parts;
    std::string cur;
    for (const char* c = path; *c; c++) {
        if (*c == '/') { if (!cur.empty()) parts.push_back(cur); cur.clear(); }
        else cur += *c;
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

// Make the directory holding a path the image's working directory (a CBM
// subdirectory, or the root); `leaf` is the path's last component
static int enterParent(const char* path, std::string& leaf) {
    auto parts = pathParts(path);
    g_img->setDirectory({0, 0});
    leaf = parts.empty() ? "" : parts.back();
    for (size_t i = 0; i + 1 < parts.size(); i++) {
        Node d;
        if (!findIn(nodes(), parts[i], d)) return -ENOENT;
        if (!isDir(d) || !d.entry.isValid()) return -ENOTDIR;
        g_img->setDirectory(d.entry.firstDataTS);
    }
    return 0;
}

// The file or directory a path names; its directory becomes the working one
static bool findNode(const char* path, Node& node) {
    std::string leaf;
    if (enterParent(path, leaf) != 0 || leaf.empty()) return false;
    return findIn(nodes(), leaf, node);
}

// The parent part of a path ("/" for a root entry)
static std::string parentOf(const std::string& path) {
    auto slash = path.rfind('/');
    return slash == 0 || slash == std::string::npos ? "/" : path.substr(0, slash);
}

static bool nodeBytes(const Node& n, std::vector<uint8_t>& data) {
    if (n.geos) {
        std::string err;
        return g_img->readGeosCvt(n.entry, data, err);
    }
    data = g_img->readFile(n.info.name);
    return true;
}

// .cvt files being written: kept here until they are closed
static std::map<std::string, std::vector<uint8_t>> g_pending;

// A closed .cvt: a GEOS file named as the path (replacing one of that name);
// data that is not a CVT is stored as a plain USR file
static int commitPending(const std::string& path) {
    auto it = g_pending.find(path);
    if (it == g_pending.end()) return 0;
    std::vector<uint8_t> data = std::move(it->second);
    g_pending.erase(it);
    std::string stem;
    if (int rc = enterParent(path.c_str(), stem)) return rc;
    stem = stem.substr(0, stem.size() - 4);  // "name.cvt" -> "name"
    Node old;
    if (findNode(path.c_str(), old)) g_img->removeFile(old.info.name);
    std::string err;
    if (DiskImage::isCvt(data)) {
        // the GEOS name is the file's name as written (ASCII, case kept)
        std::memset(data.data() + 3, 0xA0, 16);
        std::memcpy(data.data() + 3, stem.data(), std::min<size_t>(16, stem.size()));
        if (!g_img->addGeosCvt(data, "", err)) {
            fprintf(stderr, "disk45 mount: %s: %s\n", path.c_str(), err.c_str());
            return -ENOSPC;
        }
    } else {
        fprintf(stderr, "disk45 mount: %s is not a GEOS Convert file; stored as a USR file\n", path.c_str());
        std::string name = stem;
        std::transform(name.begin(), name.end(), name.begin(), ::toupper);
        if (!g_img->addFile(name, CbmFileType::USR, data)) return -ENOSPC;
    }
    return g_img->saveImage(g_imagePath) ? 0 : -EIO;
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
    auto pend = g_pending.find(path);
    if (pend != g_pending.end()) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = pend->second.size();
        return 0;
    }
    Node n;
    if (!findNode(path, n)) return -ENOENT;
    if (isDir(n)) {
        st->st_mode = S_IFDIR | (g_readonly ? 0555 : 0755);
        st->st_nlink = 2;
    } else {
        st->st_mode = S_IFREG | (g_readonly ? 0444 : 0644);
        st->st_nlink = 1;
        std::vector<uint8_t> data;
        nodeBytes(n, data);
        st->st_size = data.size();
    }
    return 0;
}

static int cbm_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                        off_t, struct fuse_file_info*, enum fuse_readdir_flags) {
    std::lock_guard<std::mutex> lock(g_mutex);

    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);

    if (strcmp(path, "/") == 0) {
        g_img->setDirectory({0, 0});
    } else {
        Node d;
        if (!findNode(path, d)) return -ENOENT;
        if (!isDir(d) || !d.entry.isValid()) return -ENOTDIR;
        g_img->setDirectory(d.entry.firstDataTS);
    }

    std::vector<std::string> shown;
    std::string dir = strcmp(path, "/") == 0 ? "" : path;
    for (auto& n : nodes()) {
        std::string posixName = cbmToPath(n.info, n.geos);
        shown.push_back(dir + "/" + posixName);
        filler(buf, posixName.c_str(), NULL, 0, (fuse_fill_dir_flags)0);
    }
    for (auto& [p, data] : g_pending)
        if (parentOf(p) == path && std::find(shown.begin(), shown.end(), p) == shown.end())
            filler(buf, p.c_str() + p.rfind('/') + 1, NULL, 0, (fuse_fill_dir_flags)0);
    return 0;
}

static int cbm_open(const char* path, struct fuse_file_info* fi) {
    std::lock_guard<std::mutex> lock(g_mutex);
    bool writing = (fi->flags & O_ACCMODE) != O_RDONLY;
    if (g_pending.count(path)) {
        if (fi->flags & O_TRUNC) g_pending[path].clear();
        return 0;
    }
    Node n;
    if (!findNode(path, n)) return -ENOENT;
    if (g_readonly && writing) return -EACCES;
    if (writing && isDir(n)) return -EISDIR;
    if (writing && n.geos) {
        // a GEOS file is rewritten as a whole CVT, committed on close
        std::vector<uint8_t> data;
        if (!(fi->flags & O_TRUNC)) nodeBytes(n, data);
        g_pending[path] = data;
    }
    return 0;
}

static int cbm_read(const char* path, char* buf, size_t size, off_t offset,
                     struct fuse_file_info*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<uint8_t> data;
    auto pend = g_pending.find(path);
    if (pend != g_pending.end()) {
        data = pend->second;
    } else {
        Node n;
        if (!findNode(path, n)) return -ENOENT;
        if (!nodeBytes(n, data)) return -EIO;
    }

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
    auto pend = g_pending.find(path);
    if (pend != g_pending.end()) {
        auto& data = pend->second;
        if ((size_t)offset + size > data.size()) data.resize(offset + size, 0);
        memcpy(data.data() + offset, buf, size);
        return (int)size;
    }
    Node n;
    std::string cbmName = findNode(path, n) ? n.info.name : pathToCbmName(path);

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
    if (!g_img->saveImage(g_imagePath)) return -EIO;
    return (int)size;
}

static int cbm_create(const char* path, mode_t, struct fuse_file_info*) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string leaf;
    if (int rc = enterParent(path, leaf)) return rc;
    std::string cbmName = pathToCbmName(path);
    if (cbmName.size() > 16) return -ENAMETOOLONG;

    if (isCvtPath(path)) {
        if (!trackImage()) return -EACCES;
        g_pending[path].clear();  // a GEOS file once it is closed
        return 0;
    }
    CbmFileType ftype = extToType(path);
    std::vector<uint8_t> empty;
    if (!g_img->addFile(cbmName, ftype, empty)) return -ENOSPC;
    return g_img->saveImage(g_imagePath) ? 0 : -EIO;
}

static int cbm_release(const char* path, struct fuse_file_info*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return commitPending(path);
}

static int cbm_unlink(const char* path) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    bool pending = g_pending.erase(path) > 0;
    Node n;
    if (!findNode(path, n)) return pending ? 0 : -ENOENT;
    if (isDir(n)) return -EISDIR;
    if (!g_img->removeFile(n.info.name)) return -ENOENT;  // a GEOS file: all its sectors
    return g_img->saveImage(g_imagePath) ? 0 : -EIO;
}

static int cbm_rename(const char* from, const char* to, unsigned int) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string toLeaf;
    if (int rc = enterParent(to, toLeaf)) return rc;
    TrackSector toDir = g_img->directoryStart();
    Node target;
    bool replace = findIn(nodes(), toLeaf, target);
    Node n;
    if (!findNode(from, n)) return -ENOENT;
    std::string newName = pathToCbmName(to);
    if (n.geos) {
        // keep the case of a GEOS name
        newName = isCvtPath(toLeaf) ? toLeaf.substr(0, toLeaf.size() - 4) : toLeaf;
    }
    if (newName.size() > 16) return -ENAMETOOLONG;
    if (replace) {
        // POSIX rename replaces the target (an empty directory by a directory)
        if (isDir(target) != isDir(n)) return isDir(target) ? -EISDIR : -ENOTDIR;
        if (target.info.name == n.info.name && toDir == g_img->directoryStart()) {
            // the same entry (a change of case)
        } else {
            TrackSector fromDir = g_img->directoryStart();
            g_img->setDirectory(toDir);
            int rc = isDir(target) ? g_img->removeDirectory(target.info.name)
                                   : (g_img->removeFile(target.info.name) ? 0 : -EIO);
            g_img->setDirectory(fromDir);
            if (rc) return rc;
        }
    }
    if (toDir == g_img->directoryStart()) {
        if (!g_img->renameFile(n.info.name, newName)) return -EIO;
    } else {
        if (isDir(n)) return -EXDEV;  // a directory is not moved (it could land inside itself)
        if (!g_img->moveFile(n.info.name, toDir, newName)) return -EIO;
    }
    return g_img->saveImage(g_imagePath) ? 0 : -EIO;
}

static int cbm_mkdir(const char* path, mode_t) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!trackImage()) return -EPERM;
    std::string leaf;
    if (int rc = enterParent(path, leaf)) return rc;
    Node n;
    if (findIn(nodes(), leaf, n)) return -EEXIST;
    std::string name = leaf;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    if (name.size() > 16) return -ENAMETOOLONG;
    if (!g_img->makeDirectory(name)) return -ENOSPC;
    return g_img->saveImage(g_imagePath) ? 0 : -EIO;
}

static int cbm_rmdir(const char* path) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    Node n;
    if (!findNode(path, n)) return -ENOENT;
    if (!isDir(n)) return -ENOTDIR;
    if (int rc = g_img->removeDirectory(n.info.name)) return rc;
    return g_img->saveImage(g_imagePath) ? 0 : -EIO;
}

static int cbm_truncate(const char* path, off_t size, struct fuse_file_info*) {
    if (g_readonly) return -EACCES;
    std::lock_guard<std::mutex> lock(g_mutex);
    auto pend = g_pending.find(path);
    if (pend != g_pending.end()) {
        pend->second.resize(size, 0);
        return 0;
    }
    Node n;
    if (!findNode(path, n)) return -ENOENT;
    if (n.geos) {
        std::vector<uint8_t> data;
        nodeBytes(n, data);
        data.resize(size, 0);
        g_pending[path] = data;
        return 0;
    }
    auto data = g_img->readFile(n.info.name);
    data.resize(size, 0);
    CbmFileType ftype = extToType(path);
    g_img->removeFile(n.info.name);
    if (!g_img->addFile(n.info.name, ftype, data)) return -ENOSPC;
    return g_img->saveImage(g_imagePath) ? 0 : -EIO;
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
    cbm_ops.release  = cbm_release;
    cbm_ops.mkdir    = cbm_mkdir;
    cbm_ops.rmdir    = cbm_rmdir;
}

// ============================================================================
// Entry point (called from disk45 mount command)
// ============================================================================

int disk45_fuse_main(const std::string& imagePath, const std::string& mountpoint,
                      bool readonly, int argc, char** argv) {
    // FUSE runs the filesystem from / : the image path must not be relative
    g_imagePath = std::filesystem::absolute(imagePath).string();
    g_readonly = readonly;

    g_img = DiskImage::load(imagePath);
    if (!g_img) {
        fprintf(stderr, "Error: failed to load %s\n", imagePath.c_str());
        return 1;
    }

    // Detect read-only formats
    if (g_img->totalTracks() == 0 || !g_img->writable()) g_readonly = true;

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
