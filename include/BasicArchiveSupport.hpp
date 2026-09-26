#ifndef BASICARCHIVESUPPORT_HPP
#define BASICARCHIVESUPPORT_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

// Archive entry (file in archive)
struct ArchiveEntry {
    std::string filename;
    std::vector<uint8_t> data;
    uint32_t crc32;
    uint32_t originalSize;
    uint32_t compressedSize;
    uint64_t timestamp;
    std::string comment;
};

// Archive metadata
struct ArchiveMetadata {
    std::string name;
    std::string description;
    std::string author;
    std::string version;
    uint32_t entryCount;
    uint64_t totalSize;
    uint64_t createdTime;
    uint64_t modifiedTime;
};

// Archive format support
class ArchiveFormat {
public:
    enum class Type {
        BASIC_ARK,      // Native BASIC archive format
        ZIP,            // ZIP format
        TAR,            // TAR format
        GZIP            // GZIP format
    };

    virtual ~ArchiveFormat() = default;

    virtual bool write(const std::string& filename, const std::vector<ArchiveEntry>& entries) = 0;
    virtual bool read(const std::string& filename, std::vector<ArchiveEntry>& entries) = 0;
    virtual std::string getExtension() const = 0;
};

// Basic ARK format (custom simple archive)
class BasicArkFormat : public ArchiveFormat {
public:
    bool write(const std::string& filename, const std::vector<ArchiveEntry>& entries) override;
    bool read(const std::string& filename, std::vector<ArchiveEntry>& entries) override;
    std::string getExtension() const override { return ".ark"; }

private:
    static const uint32_t MAGIC = 0x4241534B;  // 'BASK'
    static const uint32_t VERSION = 1;
};

// Archive manager
class BasicArchiveManager {
public:
    BasicArchiveManager() = default;

    // Create new archive
    bool createArchive(
        const std::string& filename,
        const ArchiveMetadata& metadata,
        ArchiveFormat::Type format = ArchiveFormat::Type::BASIC_ARK
    );

    // Add file to archive
    bool addFile(
        const std::string& archiveFile,
        const std::string& sourceFile,
        const std::string& entryName = ""
    );

    // Add data to archive
    bool addData(
        const std::string& archiveFile,
        const std::string& entryName,
        const std::vector<uint8_t>& data
    );

    // Extract file from archive
    bool extractFile(
        const std::string& archiveFile,
        const std::string& entryName,
        const std::string& destFile
    );

    // List archive contents
    std::vector<ArchiveEntry> listArchive(const std::string& archiveFile);

    // Get archive metadata
    ArchiveMetadata getArchiveMetadata(const std::string& archiveFile);

    // Verify archive integrity
    bool verifyArchive(const std::string& archiveFile);

private:
    std::map<ArchiveFormat::Type, std::unique_ptr<ArchiveFormat>> formats;
    void initializeFormats();
};

// Bundle manager for packaging programs with dependencies
class ProgramBundle {
public:
    struct Manifest {
        std::string programName;
        std::string programVersion;
        std::string mainFile;
        std::vector<std::string> dependencies;
        std::map<std::string, std::string> resources;
        std::string description;
        std::string author;
    };

    ProgramBundle(const std::string& name) : name(name) {}

    // Build bundle from program and resources
    bool buildBundle(
        const std::string& mainProgram,
        const std::string& outputPath,
        const Manifest& manifest
    );

    // Extract and run bundle
    bool extractAndRun(
        const std::string& bundlePath,
        const std::string& extractDir
    );

    // Generate manifest file
    std::string generateManifest(const Manifest& manifest);

    // Parse manifest file
    Manifest parseManifest(const std::string& manifestContent);

    // Add dependency
    void addDependency(const std::string& depFile, const std::string& depName);

    // Add resource
    void addResource(const std::string& resFile, const std::string& resName);

private:
    std::string name;
    std::vector<std::pair<std::string, std::string>> dependencies;
    std::map<std::string, std::string> resources;
};

// Distribution package creator
class DistributionPackage {
public:
    struct PackageConfig {
        std::string packageName;
        std::string packageVersion;
        std::vector<std::string> programs;
        std::vector<std::string> libraries;
        std::string readme;
        std::string license;
        std::string changelog;
    };

    DistributionPackage() = default;

    // Create distribution package
    bool createPackage(
        const PackageConfig& config,
        const std::string& outputPath
    );

    // Add program to package
    bool addProgram(const std::string& programFile, const std::string& category = "");

    // Add library to package
    bool addLibrary(const std::string& libraryFile);

    // Generate package metadata
    std::string generatePackageInfo(const PackageConfig& config);

    // List package contents
    struct PackageInfo {
        int programCount;
        int libraryCount;
        uint64_t totalSize;
        std::vector<std::string> programs;
        std::vector<std::string> libraries;
    };

    PackageInfo inspectPackage(const std::string& packagePath);

private:
    std::vector<std::string> programs;
    std::vector<std::string> libraries;
};

// Compression support
class CompressionHelper {
public:
    enum class CompressionLevel {
        STORE = 0,      // No compression
        FAST = 1,       // Fast compression
        BALANCED = 6,   // Balanced speed/ratio
        BEST = 9        // Best compression ratio
    };

    // Compress data
    static std::vector<uint8_t> compress(
        const std::vector<uint8_t>& data,
        CompressionLevel level = CompressionLevel::BALANCED
    );

    // Decompress data
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData);

    // Calculate CRC32
    static uint32_t calculateCRC32(const std::vector<uint8_t>& data);

    // Verify CRC32
    static bool verifyCRC32(const std::vector<uint8_t>& data, uint32_t expectedCRC);
};

#endif  // BASICARCHIVESUPPORT_HPP
