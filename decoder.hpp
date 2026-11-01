#ifndef DECODER_HPP
#define DECODER_HPP

#include <cstdint>
#include <string>
#include <vector>

bool readHuffFile(
    const std::string& inputPath,
    std::vector<std::uint8_t>& decodedData);

#endif