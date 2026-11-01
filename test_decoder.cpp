
#include "decoder.hpp"
#include "encoder.hpp"
#include "huffman_tree.hpp"

#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
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

    bool writeBytes(
        const std::string& path,
        const std::vector<uint8_t>& bytes)
    {
        std::ofstream output(path, std::ios::binary);

        if (!output)
            return false;

        output.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));

        return output.good();
    }

    bool readBytes(
        const std::string& path,
        std::vector<uint8_t>& bytes)
    {
        std::ifstream input(path, std::ios::binary);

        if (!input)
            return false;

        bytes.assign(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());

        return !input.bad();
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

    bool expectCorruptedFileRejected(
        const std::vector<uint8_t>& bytes,
        const std::string& description)
    {
        const std::string path = "decoder_corrupt_test.huff";

        if (!expect(writeBytes(path, bytes),
                    description + ": create corrupted file"))
            return false;

        std::vector<uint8_t> decoded = {99, 100, 101};

        const bool success = readHuffFile(path, decoded);

        std::remove(path.c_str());

        return expect(!success, description + ": reject corrupted file") &&
               expect(
                   decoded == std::vector<uint8_t>({99, 100, 101}),
                   description + ": preserve output on failure");
    }

    bool createCompressedBytes(
        const std::vector<uint8_t>& data,
        std::vector<uint8_t>& bytes)
    {
        const std::string path = "decoder_header_test.huff";

        if (!compressTestData(data, path))
            return false;

        const bool success = readBytes(path, bytes);
        std::remove(path.c_str());

        return success;
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

    bool testInvalidMagic()
    {
        std::vector<uint8_t> bytes;

        if (!createCompressedBytes({'A', 'B', 'A'}, bytes))
            return false;

        bytes[0] = 'X';

        return expectCorruptedFileRejected(
            bytes, "invalid magic header");
    }

    bool testUnsupportedVersion()
    {
        std::vector<uint8_t> bytes;

        if (!createCompressedBytes({'A', 'B', 'A'}, bytes))
            return false;

        bytes[4] = 2;

        return expectCorruptedFileRejected(
            bytes, "unsupported version");
    }

    bool testTruncatedHeader()
    {
        std::vector<uint8_t> bytes;

        if (!createCompressedBytes({'A', 'B', 'A'}, bytes))
            return false;

        bytes.resize(10);

        return expectCorruptedFileRejected(
            bytes, "truncated header");
    }

    bool testIncorrectFrequencyTotal()
    {
        std::vector<uint8_t> bytes;

        if (!createCompressedBytes({'A', 'B', 'A'}, bytes))
            return false;

        // First frequency starts at byte offset 24.
        ++bytes[24];

        return expectCorruptedFileRejected(
            bytes, "incorrect frequency total");
    }

    bool testDuplicateSymbols()
    {
        std::vector<uint8_t> bytes;

        if (!createCompressedBytes({'A', 'B', 'A'}, bytes))
            return false;

        // First and second frequency records start at offsets 23 and 32.
        bytes[32] = bytes[23];

        return expectCorruptedFileRejected(
            bytes, "duplicate symbols");
    }

    bool testEmptyFileWithTrailingData()
    {
        std::vector<uint8_t> bytes;

        if (!createCompressedBytes({}, bytes))
            return false;

        bytes.push_back(0x00);

        return expectCorruptedFileRejected(
            bytes, "empty file with trailing data");
    }
}

int main()
{
    bool passed = true;

    std::cout << "Running decoder tests..." << std::endl;

    passed &= expect(testNormalText(), "normal text test");
    passed &= expect(testEmptyFile(), "empty file test");
    passed &= expect(testSingleSymbol(), "single-symbol test");
    passed &= expect(testBinaryData(), "binary data test");

    passed &= expect(testInvalidMagic(), "invalid magic test");
    passed &= expect(testUnsupportedVersion(), "unsupported version test");
    passed &= expect(testTruncatedHeader(), "truncated header test");
    passed &= expect(testIncorrectFrequencyTotal(), "frequency total test");
    passed &= expect(testDuplicateSymbols(), "duplicate symbols test");
    passed &= expect(
        testEmptyFileWithTrailingData(),
        "empty file trailing data test");

    if (!passed)
    {
        std::cerr << "Some decoder tests failed." << std::endl;
        return 1;
    }

    std::cout << "All decoder tests passed." << std::endl;

    return 0;
}
