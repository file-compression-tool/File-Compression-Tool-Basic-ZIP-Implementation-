#ifndef DECOMPRESSOR_HPP
#define DECOMPRESSOR_HPP

#include <string>

// Decompresses a version 1 .huff file.
// Returns true on success and false on failure.
bool decompressFile(
    const std::string& inputPath,
    const std::string& outputPath,
    bool overwriteExisting = false
);

#endif