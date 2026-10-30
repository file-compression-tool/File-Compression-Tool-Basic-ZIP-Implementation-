
#include "decompressor.hpp"
#include "huffman_tree.hpp"


#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <exception>

namespace
{
    // Reads an unsigned integer stored in little-endian format.
    template <typename T>
    bool readLittleEndian(std::ifstream& input, T& value)
    {
        value = 0;

        for (std::size_t i = 0; i < sizeof(T); ++i)
        {
            char byte;

            if (!input.get(byte))
                return false;

            value |= static_cast<T>(
                static_cast<unsigned char>(byte)
            ) << (8 * i);
        }

        return true;
    }
}

bool decompressFile(
    const std::string& inputPath,
    const std::string& outputPath,
    bool overwriteExisting)
{
    std::ifstream input(inputPath, std::ios::binary);

    if (!input)
    {
        std::cerr << "Error: Could not open compressed file.\n";
        return false;
    }

    // Read and validate magic signature.
    char magic[4];

    if (!input.read(magic, 4) ||
        magic[0] != 'H' ||
        magic[1] != 'U' ||
        magic[2] != 'F' ||
        magic[3] != 'F')
    {
        std::cerr << "Error: Invalid HUFF file signature.\n";
        return false;
    }

    // Read and validate version.
    unsigned char version;

    if (!input.read(reinterpret_cast<char*>(&version), 1) ||
        version != 1)
    {
        std::cerr << "Error: Unsupported or invalid file version.\n";
        return false;
    }

    // Read header fields.
    std::uint64_t originalSize = 0;
    std::uint64_t bitCount = 0;
    std::uint16_t symbolCount = 0;

    if (!readLittleEndian(input, originalSize) ||
        !readLittleEndian(input, bitCount) ||
        !readLittleEndian(input, symbolCount))
    {
        std::cerr << "Error: Incomplete file header.\n";
        return false;
    }

    if (symbolCount > 256)
    {
        std::cerr << "Error: Invalid symbol count.\n";
        return false;
    }

    // Read frequency table.
    std::map<std::uint8_t, std::uint64_t> frequencies;
    std::uint64_t frequencyTotal = 0;

    for (std::uint16_t i = 0; i < symbolCount; ++i)
    {
        unsigned char symbol;
        std::uint64_t frequency = 0;

        if (!input.read(reinterpret_cast<char*>(&symbol), 1) ||
            !readLittleEndian(input, frequency))
        {
            std::cerr << "Error: Incomplete frequency table.\n";
            return false;
        }

        if (frequency == 0 || frequencies.count(symbol) != 0)
        {
            std::cerr << "Error: Invalid or duplicate frequency entry.\n";
            return false;
        }

        if (frequencyTotal >
            std::numeric_limits<std::uint64_t>::max() - frequency)
        {
            std::cerr << "Error: Frequency total overflow.\n";
            return false;
        }

        frequencyTotal += frequency;
        frequencies[symbol] = frequency;
    }

    // Validate consistency of header metadata.
if (frequencyTotal != originalSize)
{
    std::cerr << "Error: Frequency total does not match original size.\n";
    return false;
}

if (originalSize == 0)
{
    if (symbolCount != 0 || bitCount != 0)
    {
        std::cerr << "Error: Invalid empty-file metadata.\n";
        return false;
    }
}
else
{
    if (symbolCount == 0)
    {
        std::cerr << "Error: Missing frequency table.\n";
        return false;
    }

    try
    {
        auto root = buildHuffmanTree(frequencies);
        auto codes = generateHuffmanCodes(root);

        std::uint64_t expectedBitCount = 0;

        for (const auto& entry : frequencies)
        {
            const auto code = codes.find(entry.first);

            if (code == codes.end() || code->second.empty())
                return false;

            std::uint64_t codeLength = code->second.size();

            if (entry.second >
                (std::numeric_limits<std::uint64_t>::max() -
                 expectedBitCount) / codeLength)
            {
                std::cerr << "Error: Expected bit count overflow.\n";
                return false;
            }

            expectedBitCount += entry.second * codeLength;
        }

        if (expectedBitCount != bitCount)
        {
            std::cerr << "Error: Encoded bit count does not match metadata.\n";
            return false;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: Invalid Huffman frequency table: "
                  << error.what() << '\n';
        return false;
    }
}

   // Rebuild the Huffman tree.
std::shared_ptr<HuffmanNode> root;

try
{
    if (originalSize > 0)
        root = buildHuffmanTree(frequencies);
}
catch (const std::exception& error)
{
    std::cerr << "Error: Could not build Huffman tree: "
              << error.what() << '\n';
    return false;
}

// Calculate the exact number of payload bytes.
std::uint64_t payloadSize =
    bitCount / 8 + (bitCount % 8 != 0);

// Read the payload.
std::vector<unsigned char> payload;

if (payloadSize > payload.max_size())
{
    std::cerr << "Error: Payload is too large.\n";
    return false;
}

payload.resize(static_cast<std::size_t>(payloadSize));

for (std::size_t i = 0; i < payload.size(); ++i)
{
    char byte;

    if (!input.get(byte))
    {
        std::cerr << "Error: Incomplete payload.\n";
        return false;
    }

    payload[i] = static_cast<unsigned char>(byte);
}

// Reject unexpected extra bytes.
if (input.peek() != EOF)
{
    std::cerr << "Error: Unexpected extra data after payload.\n";
    return false;
}

// Check that unused padding bits are zero.
if (bitCount % 8 != 0 && !payload.empty())
{
    unsigned int unusedBits =
        8 - static_cast<unsigned int>(bitCount % 8);

    unsigned char paddingMask =
        static_cast<unsigned char>((1U << unusedBits) - 1);

    if ((payload.back() & paddingMask) != 0)
    {
        std::cerr << "Error: Invalid payload padding.\n";
        return false;
    }
}

// Decode the payload.
std::vector<std::uint8_t> decodedData;
std::map<std::uint8_t, std::uint64_t> decodedFrequencies;

if (originalSize > decodedData.max_size() ||
    originalSize > std::numeric_limits<std::size_t>::max())
{
    std::cerr << "Error: Original file is too large.\n";
    return false;
}

decodedData.reserve(static_cast<std::size_t>(originalSize));

if (originalSize > 0 && root->isLeaf())
{
    // A single-symbol file must contain only zero bits.
    if (bitCount != originalSize)
    {
        std::cerr << "Error: Invalid single-symbol bit count.\n";
        return false;
    }

    for (std::uint64_t i = 0; i < bitCount; ++i)
    {
        unsigned char byte = payload[i / 8];
        unsigned int bitPosition = 7 - (i % 8);

        if ((byte >> bitPosition) & 1U)
        {
            std::cerr << "Error: Invalid single-symbol payload.\n";
            return false;
        }

        decodedData.push_back(root->symbol);
        ++decodedFrequencies[root->symbol];
    }
}
else if (originalSize > 0)
{
    auto current = root;

    for (std::uint64_t i = 0; i < bitCount; ++i)
    {
        unsigned char byte = payload[i / 8];
        unsigned int bitPosition = 7 - (i % 8);
        bool bit = (byte >> bitPosition) & 1U;

        current = bit ? current->right : current->left;

        if (!current)
        {
            std::cerr << "Error: Invalid Huffman bitstream.\n";
            return false;
        }

        if (current->isLeaf())
        {
            decodedData.push_back(current->symbol);
            ++decodedFrequencies[current->symbol];
            current = root;

            if (decodedData.size() > originalSize)
            {
                std::cerr << "Error: Decoded data exceeds original size.\n";
                return false;
            }
        }
    }

    if (current != root)
    {
        std::cerr << "Error: Incomplete Huffman code.\n";
        return false;
    }
}

// Verify decoded size and symbol frequencies.
if (decodedData.size() != originalSize ||
    decodedFrequencies != frequencies)
{
    std::cerr << "Error: Decoded data does not match header metadata.\n";
    return false;
}

// Do not overwrite the compressed input file.
if (inputPath == outputPath)
{
    std::cerr << "Error: Input and output paths must be different.\n";
    return false;
}

// Check whether the output already exists.
if (!overwriteExisting)
{
    std::ifstream existing(outputPath, std::ios::binary);

    if (existing.is_open())
    {
        std::cerr << "Error: Output file already exists.\n";
        return false;
    }
}

// Write the verified decompressed data.
std::ofstream output(
    outputPath,
    std::ios::binary | std::ios::trunc
);

if (!output)
{
    std::cerr << "Error: Could not open output file.\n";
    return false;
}

if (!decodedData.empty())
{
    output.write(
        reinterpret_cast<const char*>(decodedData.data()),
        static_cast<std::streamsize>(decodedData.size())
    );
}

if (!output)
{
    std::cerr << "Error: Failed to write output file.\n";
    return false;
}

output.close();

return !output.fail();
}
