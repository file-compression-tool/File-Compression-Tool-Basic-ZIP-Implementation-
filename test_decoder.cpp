
#include "decoder.hpp"
#include "encoder.hpp"
#include "huffman_tree.hpp"

#include <cstdio>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

using std::uint8_t;

namespace
{
    bool expect(bool condition, const std::string& description)
    {
        if (!condition)
            std::cerr << "FAILED: " << description << std::endl;

        return condition;
    }

    bool compressTestData(
        const std::vector<uint8_t>& data,
        const std::string& path)
    {
        std::map<uint8_t, std::uint64_t> frequencies;

        for (uint8_t symbol : data)
            ++frequencies[symbol];

        const auto tree = buildHuffmanTree(frequencies);
        const auto generatedCodes = generateHuffmanCodes(tree);

        const std::unordered_map<uint8_t, std::string> codes(
            generatedCodes.begin(), generatedCodes.end());

        return writeHuffFile(data, codes, path, true);
    }

    bool testRoundTrip(
        const std::vector<uint8_t>& original,
        const std::string& path,
        const std::string& description)
    {
        if (!expect(compressTestData(original, path),
                    description + ": create compressed file"))
            return false;

        std::vector<uint8_t> decoded;

        const bool success = readHuffFile(path, decoded);

        std::remove(path.c_str());

        return expect(success, description + ": decode successfully") &&
               expect(decoded == original,
                      description + ": reproduce original bytes");
    }

    bool testNormalText()
    {
        const std::string message = "Hello Huffman Decoder!";

        return testRoundTrip(
            std::vector<uint8_t>(message.begin(), message.end()),
            "decoder_text_test.huff",
            "normal text");
    }

    bool testEmptyFile()
    {
        return testRoundTrip(
            {},
            "decoder_empty_test.huff",
            "empty file");
    }

    bool testSingleSymbol()
    {
        return testRoundTrip(
            std::vector<uint8_t>(100, 'A'),
            "decoder_single_test.huff",
            "single-symbol file");
    }

    bool testBinaryData()
    {
        std::vector<uint8_t> data;

        for (int i = 0; i < 256; ++i)
            data.push_back(static_cast<uint8_t>(i));

        return testRoundTrip(
            data,
            "decoder_binary_test.huff",
            "binary file containing all 256 byte values");
    }
}

int main()
{
    const bool passed =
        testNormalText() &&
        testEmptyFile() &&
        testSingleSymbol() &&
        testBinaryData();

    if (!passed)
        return 1;

    std::cout << "All decoder tests passed." << std::endl;

    return 0;
}
