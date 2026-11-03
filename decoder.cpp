
#include "decoder.hpp"

#include "huffman_tree.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
    bool readByte(std::istream& input, std::uint8_t& value)
    {
        char byte = 0;

        if (!input.get(byte))
            return false;

        value = static_cast<std::uint8_t>(
            static_cast<unsigned char>(byte));

        return true;
    }

    bool readLittleEndian(
        std::istream& input,
        std::uint64_t& value,
        unsigned int byteCount)
    {
        value = 0;

        for (unsigned int i = 0; i < byteCount; ++i)
        {
            std::uint8_t byte = 0;

            if (!readByte(input, byte))
                return false;

            value |= static_cast<std::uint64_t>(byte) << (8 * i);
        }

        return true;
    }

    bool readHeader(
        std::istream& input,
        std::uint64_t& originalSize,
        std::uint64_t& bitCount,
        std::map<std::uint8_t, std::uint64_t>& frequencies)
    {
        char magic[4] = {};

        if (!input.read(magic, 4))
            return false;

        if (magic[0] != 'H' || magic[1] != 'U' ||
            magic[2] != 'F' || magic[3] != 'F')
            return false;

        std::uint8_t version = 0;

        if (!readByte(input, version) || version != 1)
            return false;

        if (!readLittleEndian(input, originalSize, 8) ||
            !readLittleEndian(input, bitCount, 8))
            return false;

        std::uint64_t symbolCount = 0;

        if (!readLittleEndian(input, symbolCount, 2))
            return false;

        if (symbolCount > 256)
            return false;

        std::uint64_t frequencyTotal = 0;

        for (std::uint64_t i = 0; i < symbolCount; ++i)
        {
            std::uint8_t symbol = 0;
            std::uint64_t frequency = 0;

            if (!readByte(input, symbol) ||
                !readLittleEndian(input, frequency, 8))
                return false;

            if (frequency == 0 ||
                frequencies.find(symbol) != frequencies.end())
                return false;

            if (frequency >
                std::numeric_limits<std::uint64_t>::max() -
                    frequencyTotal)
                return false;

            frequencyTotal += frequency;
            frequencies[symbol] = frequency;
        }

        if (frequencyTotal != originalSize)
            return false;

        if ((originalSize == 0) != (symbolCount == 0))
            return false;

        if (originalSize == 0 && bitCount != 0)
            return false;

        if (originalSize > 0 && bitCount == 0)
            return false;

        return true;
    }

    // Calculate the expected bit count directly from Huffman tree depths.
    bool validateBitCount(
        const std::shared_ptr<HuffmanNode>& root,
        const std::map<std::uint8_t, std::uint64_t>& frequencies,
        std::uint64_t bitCount)
    {
        if (!root)
            return frequencies.empty() && bitCount == 0;

        struct PendingNode
        {
            std::shared_ptr<HuffmanNode> node;
            std::uint64_t depth;
        };

        std::vector<PendingNode> pending;
        pending.push_back({root, 0});

        std::uint64_t expectedBits = 0;
        std::array<bool, 256> visitedSymbols = {};

        while (!pending.empty())
        {
            const PendingNode current = pending.back();
            pending.pop_back();

            if (!current.node)
                return false;

            if (current.node->isLeaf())
            {
                const std::uint8_t symbol = current.node->symbol;

                const auto frequency = frequencies.find(symbol);

                if (frequency == frequencies.end() ||
                    visitedSymbols[symbol])
                    return false;

                visitedSymbols[symbol] = true;

                // A single-symbol tree uses the code "0".
                const std::uint64_t codeLength =
                    current.depth == 0 ? 1 : current.depth;

                if (frequency->second >
                    (std::numeric_limits<std::uint64_t>::max() -
                     expectedBits) / codeLength)
                    return false;

                expectedBits += frequency->second * codeLength;
            }
            else
            {
                if (!current.node->left || !current.node->right)
                    return false;

                if (current.depth ==
                    std::numeric_limits<std::uint64_t>::max())
                    return false;

                const std::uint64_t nextDepth = current.depth + 1;

                pending.push_back({current.node->right, nextDepth});
                pending.push_back({current.node->left, nextDepth});
            }
        }

        // Ensure every symbol in the header exists in the tree.
        for (const auto& entry : frequencies)
        {
            if (!visitedSymbols[entry.first])
                return false;
        }

        return expectedBits == bitCount;
    }

    bool decodeBitstream(
        std::istream& input,
        const std::shared_ptr<HuffmanNode>& root,
        std::uint64_t originalSize,
        std::uint64_t bitCount,
        const std::map<std::uint8_t, std::uint64_t>& frequencies,
        std::vector<std::uint8_t>& decodedData)
    {
        const std::uint64_t payloadBytes =
            bitCount / 8 + (bitCount % 8 != 0 ? 1 : 0);

        if (payloadBytes >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()))
            return false;

        if (originalSize >
            static_cast<std::uint64_t>(decodedData.max_size()))
            return false;

        std::vector<std::uint8_t> result;
        result.reserve(static_cast<std::size_t>(originalSize));

        std::array<std::uint64_t, 256> decodedFrequencies = {};

        auto current = root;

        std::uint64_t processedBits = 0;

        for (std::uint64_t byteIndex = 0;
             byteIndex < payloadBytes;
             ++byteIndex)
        {
            std::uint8_t packedByte = 0;

            if (!readByte(input, packedByte))
                return false;

            const unsigned int bitsThisByte =
                static_cast<unsigned int>(
                    bitCount - processedBits < 8
                        ? bitCount - processedBits
                        : 8);

            for (unsigned int bitIndex = 0;
                 bitIndex < bitsThisByte;
                 ++bitIndex)
            {
                const bool bit =
                    (packedByte & (0x80 >> bitIndex)) != 0;

                if (!root)
                    return false;

                if (root->isLeaf())
                {
                    if (bit)
                        return false;

                    if (result.size() >= originalSize)
                        return false;

                    result.push_back(root->symbol);
                    ++decodedFrequencies[root->symbol];
                }
                else
                {
                    current = bit ? current->right : current->left;

                    if (!current)
                        return false;

                    if (current->isLeaf())
                    {
                        if (result.size() >= originalSize)
                            return false;

                        result.push_back(current->symbol);
                        ++decodedFrequencies[current->symbol];

                        current = root;
                    }
                }

                ++processedBits;
            }

            // The unused padding bits must be zero.
            if (bitsThisByte < 8)
            {
                const unsigned int unusedBits = 8 - bitsThisByte;
                const std::uint8_t paddingMask =
                    static_cast<std::uint8_t>(
                        (1u << unusedBits) - 1u);

                if ((packedByte & paddingMask) != 0)
                    return false;
            }
        }

        // Reject unexpected trailing bytes, including for empty files.
        char extraByte = 0;

        if (input.get(extraByte))
            return false;

        if (!input.eof())
            return false;

        // An empty tree is valid only for an empty file with no payload.
        if (!root)
            return originalSize == 0 &&
                   bitCount == 0 &&
                   payloadBytes == 0;

        if (processedBits != bitCount)
            return false;

        if (!root->isLeaf() && current != root)
            return false;

        if (result.size() != originalSize)
            return false;

        // Verify that decoded symbol frequencies match the header.
        for (const auto& entry : frequencies)
        {
            if (decodedFrequencies[entry.first] != entry.second)
                return false;
        }

        decodedData.swap(result);
        return true;
    }
}

bool readHuffFile(
    const std::string& inputPath,
    std::vector<std::uint8_t>& decodedData)
{
    std::ifstream input(inputPath.c_str(), std::ios::binary);

    if (!input.is_open())
    {
        std::cerr << "Error: Could not open compressed file: "
                  << inputPath << std::endl;
        return false;
    }

    try
    {
        std::uint64_t originalSize = 0;
        std::uint64_t bitCount = 0;

        std::map<std::uint8_t, std::uint64_t> frequencies;

        if (!readHeader(input, originalSize, bitCount, frequencies))
        {
            std::cerr << "Error: Invalid or corrupted .huff header."
                      << std::endl;
            return false;
        }

        // Reconstruct the Huffman tree from stored frequencies.
        const std::shared_ptr<HuffmanNode> root =
            buildHuffmanTree(frequencies);

        // Validate bit count directly using tree depths.
        if (!validateBitCount(root, frequencies, bitCount))
        {
            std::cerr << "Error: Inconsistent compressed bit count."
                      << std::endl;
            return false;
        }

        std::vector<std::uint8_t> result;

        if (!decodeBitstream(
                input,
                root,
                originalSize,
                bitCount,
                frequencies,
                result))
        {
            std::cerr << "Error: Invalid or corrupted compressed bitstream."
                      << std::endl;
            return false;
        }

        decodedData.swap(result);
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: Decompression failed: "
                  << error.what() << std::endl;
        return false;
    }
}
