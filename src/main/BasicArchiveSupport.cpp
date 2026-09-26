#include "BasicArchiveSupport.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstring>

// ==================== BasicArkFormat ====================

// Define static members
const uint32_t BasicArkFormat::MAGIC;
const uint32_t BasicArkFormat::VERSION;

bool BasicArkFormat::write(const std::string& filename, const std::vector<ArchiveEntry>& entries) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    // Write magic and version
    file.write(reinterpret_cast<const char*>(&MAGIC), sizeof(MAGIC));
    file.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));

    // Write entry count
    uint32_t entryCount = entries.size();
    file.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));

    // Write each entry
    for (const auto& entry : entries) {
        // Write filename length and data
        uint32_t nameLen = entry.filename.length();
        file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        file.write(entry.filename.c_str(), nameLen);

        // Write data length and data
        uint32_t dataLen = entry.data.size();
        file.write(reinterpret_cast<const char*>(&dataLen), sizeof(dataLen));
        file.write(reinterpret_cast<const char*>(entry.data.data()), dataLen);

        // Write metadata
        file.write(reinterpret_cast<const char*>(&entry.crc32), sizeof(entry.crc32));
        file.write(reinterpret_cast<const char*>(&entry.originalSize), sizeof(entry.originalSize));
        file.write(reinterpret_cast<const char*>(&entry.timestamp), sizeof(entry.timestamp));

        // Write comment length and data
        uint32_t commentLen = entry.comment.length();
        file.write(reinterpret_cast<const char*>(&commentLen), sizeof(commentLen));
        if (commentLen > 0) {
            file.write(entry.comment.c_str(), commentLen);
        }
    }

    return file.good();
}

bool BasicArkFormat::read(const std::string& filename, std::vector<ArchiveEntry>& entries) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    // Read and verify magic and version
    uint32_t magic, version;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != MAGIC || version != VERSION) {
        return false;
    }

    // Read entry count
    uint32_t entryCount;
    file.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));

    // Read each entry
    entries.clear();
    for (uint32_t i = 0; i < entryCount; i++) {
        ArchiveEntry entry;

        // Read filename
        uint32_t nameLen;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        entry.filename.resize(nameLen);
        file.read(&entry.filename[0], nameLen);

        // Read data
        uint32_t dataLen;
        file.read(reinterpret_cast<char*>(&dataLen), sizeof(dataLen));
        entry.data.resize(dataLen);
        file.read(reinterpret_cast<char*>(entry.data.data()), dataLen);

        // Read metadata
        file.read(reinterpret_cast<char*>(&entry.crc32), sizeof(entry.crc32));
        file.read(reinterpret_cast<char*>(&entry.originalSize), sizeof(entry.originalSize));
        file.read(reinterpret_cast<char*>(&entry.timestamp), sizeof(entry.timestamp));

        // Read comment
        uint32_t commentLen;
        file.read(reinterpret_cast<char*>(&commentLen), sizeof(commentLen));
        if (commentLen > 0) {
            entry.comment.resize(commentLen);
            file.read(&entry.comment[0], commentLen);
        }

        entries.push_back(entry);
    }

    return file.good();
}

// ==================== BasicArchiveManager ====================

void BasicArchiveManager::initializeFormats() {
    formats[ArchiveFormat::Type::BASIC_ARK] = std::make_unique<BasicArkFormat>();
}

bool BasicArchiveManager::createArchive(
    const std::string& filename,
    const ArchiveMetadata& metadata,
    ArchiveFormat::Type format
) {
    if (formats.empty()) {
        initializeFormats();
    }

    auto it = formats.find(format);
    if (it == formats.end()) {
        return false;
    }

    // Create empty archive
    std::vector<ArchiveEntry> entries;
    return it->second->write(filename, entries);
}

bool BasicArchiveManager::addFile(
    const std::string& archiveFile,
    const std::string& sourceFile,
    const std::string& entryName
) {
    std::ifstream file(sourceFile, std::ios::binary);
    if (!file) return false;

    // Read file data
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    file.close();

    // Use source filename if entry name not provided
    std::string name = entryName.empty() ? sourceFile : entryName;

    // Add to archive
    return addData(archiveFile, name, data);
}

bool BasicArchiveManager::addData(
    const std::string& archiveFile,
    const std::string& entryName,
    const std::vector<uint8_t>& data
) {
    // Read existing archive
    std::vector<ArchiveEntry> entries;
    if (!listArchive(archiveFile).empty()) {
        // Archive exists, read it
        BasicArkFormat fmt;
        if (!fmt.read(archiveFile, entries)) {
            return false;
        }
    }

    // Create new entry
    ArchiveEntry entry;
    entry.filename = entryName;
    entry.data = data;
    entry.originalSize = data.size();
    entry.compressedSize = data.size();
    entry.crc32 = CompressionHelper::calculateCRC32(data);
    entry.timestamp = std::time(nullptr);

    entries.push_back(entry);

    // Write archive
    BasicArkFormat fmt;
    return fmt.write(archiveFile, entries);
}

bool BasicArchiveManager::extractFile(
    const std::string& archiveFile,
    const std::string& entryName,
    const std::string& destFile
) {
    auto entries = listArchive(archiveFile);

    for (const auto& entry : entries) {
        if (entry.filename == entryName) {
            std::ofstream file(destFile, std::ios::binary);
            if (!file) return false;

            file.write(reinterpret_cast<const char*>(entry.data.data()), entry.data.size());
            return file.good();
        }
    }

    return false;
}

std::vector<ArchiveEntry> BasicArchiveManager::listArchive(const std::string& archiveFile) {
    std::vector<ArchiveEntry> entries;
    BasicArkFormat fmt;
    fmt.read(archiveFile, entries);
    return entries;
}

ArchiveMetadata BasicArchiveManager::getArchiveMetadata(const std::string& archiveFile) {
    ArchiveMetadata metadata;
    auto entries = listArchive(archiveFile);

    metadata.entryCount = entries.size();
    metadata.totalSize = 0;

    for (const auto& entry : entries) {
        metadata.totalSize += entry.compressedSize;
    }

    return metadata;
}

bool BasicArchiveManager::verifyArchive(const std::string& archiveFile) {
    auto entries = listArchive(archiveFile);

    for (const auto& entry : entries) {
        if (!CompressionHelper::verifyCRC32(entry.data, entry.crc32)) {
            return false;
        }
    }

    return true;
}

// ==================== ProgramBundle ====================

bool ProgramBundle::buildBundle(
    const std::string& mainProgram,
    const std::string& outputPath,
    const Manifest& manifest
) {
    BasicArchiveManager archiveMgr;

    // Create archive
    ArchiveMetadata archiveMetadata;
    archiveMetadata.name = manifest.programName;
    archiveMetadata.description = manifest.description;
    archiveMetadata.author = manifest.author;
    archiveMetadata.version = manifest.programVersion;

    if (!archiveMgr.createArchive(outputPath, archiveMetadata)) {
        return false;
    }

    // Add main program
    if (!archiveMgr.addFile(outputPath, mainProgram, manifest.mainFile)) {
        return false;
    }

    // Add dependencies
    for (const auto& [depFile, depName] : dependencies) {
        if (!archiveMgr.addFile(outputPath, depFile, depName)) {
            return false;
        }
    }

    // Add resources
    for (const auto& [resName, resFile] : resources) {
        std::ifstream file(resFile, std::ios::binary);
        if (file) {
            std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
            archiveMgr.addData(outputPath, resName, data);
        }
    }

    // Add manifest
    std::string manifestContent = generateManifest(manifest);
    std::vector<uint8_t> manifestData(manifestContent.begin(), manifestContent.end());
    archiveMgr.addData(outputPath, "MANIFEST.txt", manifestData);

    return true;
}

bool ProgramBundle::extractAndRun(
    const std::string& bundlePath,
    const std::string& extractDir
) {
    BasicArchiveManager archiveMgr;
    auto entries = archiveMgr.listArchive(bundlePath);

    // Extract all files
    for (const auto& entry : entries) {
        std::string destPath = extractDir + "/" + entry.filename;
        if (!archiveMgr.extractFile(bundlePath, entry.filename, destPath)) {
            return false;
        }
    }

    return true;
}

std::string ProgramBundle::generateManifest(const Manifest& manifest) {
    std::ostringstream manifest_str;

    manifest_str << "PROGRAM: " << manifest.programName << "\n";
    manifest_str << "VERSION: " << manifest.programVersion << "\n";
    manifest_str << "AUTHOR: " << manifest.author << "\n";
    manifest_str << "DESCRIPTION: " << manifest.description << "\n";
    manifest_str << "MAIN_FILE: " << manifest.mainFile << "\n";

    if (!manifest.dependencies.empty()) {
        manifest_str << "DEPENDENCIES:\n";
        for (const auto& dep : manifest.dependencies) {
            manifest_str << "  " << dep << "\n";
        }
    }

    if (!manifest.resources.empty()) {
        manifest_str << "RESOURCES:\n";
        for (const auto& [name, path] : manifest.resources) {
            manifest_str << "  " << name << ": " << path << "\n";
        }
    }

    return manifest_str.str();
}

ProgramBundle::Manifest ProgramBundle::parseManifest(const std::string& manifestContent) {
    Manifest manifest;
    std::istringstream iss(manifestContent);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.find("PROGRAM: ") == 0) {
            manifest.programName = line.substr(9);
        } else if (line.find("VERSION: ") == 0) {
            manifest.programVersion = line.substr(9);
        } else if (line.find("AUTHOR: ") == 0) {
            manifest.author = line.substr(8);
        } else if (line.find("DESCRIPTION: ") == 0) {
            manifest.description = line.substr(13);
        } else if (line.find("MAIN_FILE: ") == 0) {
            manifest.mainFile = line.substr(11);
        }
    }

    return manifest;
}

void ProgramBundle::addDependency(const std::string& depFile, const std::string& depName) {
    dependencies.emplace_back(depFile, depName);
}

void ProgramBundle::addResource(const std::string& resFile, const std::string& resName) {
    resources[resName] = resFile;
}

// ==================== DistributionPackage ====================

bool DistributionPackage::createPackage(
    const PackageConfig& config,
    const std::string& outputPath
) {
    BasicArchiveManager archiveMgr;

    // Create archive
    ArchiveMetadata metadata;
    metadata.name = config.packageName;
    metadata.version = config.packageVersion;

    if (!archiveMgr.createArchive(outputPath, metadata)) {
        return false;
    }

    // Add programs
    for (const auto& program : config.programs) {
        if (!archiveMgr.addFile(outputPath, program)) {
            return false;
        }
    }

    // Add libraries
    for (const auto& library : config.libraries) {
        if (!archiveMgr.addFile(outputPath, library)) {
            return false;
        }
    }

    // Add documentation
    if (!config.readme.empty()) {
        archiveMgr.addFile(outputPath, config.readme, "README.txt");
    }
    if (!config.license.empty()) {
        archiveMgr.addFile(outputPath, config.license, "LICENSE.txt");
    }
    if (!config.changelog.empty()) {
        archiveMgr.addFile(outputPath, config.changelog, "CHANGELOG.txt");
    }

    // Add package info
    std::string pkgInfo = generatePackageInfo(config);
    std::vector<uint8_t> pkgData(pkgInfo.begin(), pkgInfo.end());
    archiveMgr.addData(outputPath, "PACKAGE_INFO.txt", pkgData);

    return true;
}

bool DistributionPackage::addProgram(const std::string& programFile, const std::string& category) {
    programs.push_back(category.empty() ? programFile : (category + "/" + programFile));
    return true;
}

bool DistributionPackage::addLibrary(const std::string& libraryFile) {
    libraries.push_back(libraryFile);
    return true;
}

std::string DistributionPackage::generatePackageInfo(const PackageConfig& config) {
    std::ostringstream info;

    info << "PACKAGE_NAME: " << config.packageName << "\n";
    info << "PACKAGE_VERSION: " << config.packageVersion << "\n";
    info << "PROGRAMS: " << config.programs.size() << "\n";
    info << "LIBRARIES: " << config.libraries.size() << "\n";

    return info.str();
}

DistributionPackage::PackageInfo DistributionPackage::inspectPackage(const std::string& packagePath) {
    PackageInfo info;
    BasicArchiveManager archiveMgr;

    auto entries = archiveMgr.listArchive(packagePath);

    info.programCount = 0;
    info.libraryCount = 0;
    info.totalSize = 0;

    for (const auto& entry : entries) {
        info.totalSize += entry.compressedSize;

        if (entry.filename.find(".prg") != std::string::npos) {
            info.programCount++;
            info.programs.push_back(entry.filename);
        } else if (entry.filename.find(".lib") != std::string::npos) {
            info.libraryCount++;
            info.libraries.push_back(entry.filename);
        }
    }

    return info;
}

// ==================== CompressionHelper ====================

std::vector<uint8_t> CompressionHelper::compress(
    const std::vector<uint8_t>& data,
    CompressionLevel level
) {
    // For now, store uncompressed (level 0)
    if (level == CompressionLevel::STORE) {
        return data;
    }

    // TODO: Implement actual compression (deflate/zlib)
    return data;
}

std::vector<uint8_t> CompressionHelper::decompress(const std::vector<uint8_t>& compressedData) {
    // For now, assume stored uncompressed
    // TODO: Implement actual decompression
    return compressedData;
}

uint32_t CompressionHelper::calculateCRC32(const std::vector<uint8_t>& data) {
    uint32_t crc = 0xFFFFFFFF;

    for (uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; i++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

bool CompressionHelper::verifyCRC32(const std::vector<uint8_t>& data, uint32_t expectedCRC) {
    return calculateCRC32(data) == expectedCRC;
}
