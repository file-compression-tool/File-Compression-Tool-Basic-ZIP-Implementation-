#include "decompressor.hpp"
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

int main()
{
    const std::string compressedPath = "decompressor_test_fixture.huff";
    const std::string outputPath = "decompressor_test_existing_output.txt";
    const std::string existingContents = "preserve existing output";

    {
        std::ifstream compressed(compressedPath.c_str(), std::ios::binary);
        std::ifstream output(outputPath.c_str(), std::ios::binary);

        if (compressed.is_open() || output.is_open())
        {
            std::cerr << "FAIL: Test fixture path already exists.\n";
            return 1;
        }
    }

    const std::vector<std::uint8_t> original = {
        'd', 'e', 'c', 'o', 'm', 'p', 'r', 'e', 's', 's', 'o', 'r'
    };

    std::map<std::uint8_t, std::uint64_t> frequencies;

    for (std::uint8_t symbol : original)
        ++frequencies[symbol];

    const auto tree = buildHuffmanTree(frequencies);
    const HuffmanCodeMap generatedCodes = generateHuffmanCodes(tree);
    const std::unordered_map<std::uint8_t, std::string> codes(
        generatedCodes.begin(), generatedCodes.end());

    bool fixtureCreated =
        writeHuffFile(original, codes, compressedPath, false);

    if (!fixtureCreated)
    {
        std::cerr << "FAIL: Could not create compressed test fixture.\n";
        std::remove(compressedPath.c_str());
        return 1;
    }

    {
        std::ofstream output(outputPath.c_str(), std::ios::binary);
        output.write(
            existingContents.data(),
            static_cast<std::streamsize>(existingContents.size()));

        if (!output)
        {
            std::cerr << "FAIL: Could not create existing output fixture.\n";
            output.close();
            std::remove(compressedPath.c_str());
            std::remove(outputPath.c_str());
            return 1;
        }
    }

    const bool decompressionRejected =
        !decompressFile(compressedPath, outputPath);

    std::ifstream output(outputPath.c_str(), std::ios::binary);
    const bool outputOpened = output.is_open();
    const std::istreambuf_iterator<char> begin(output);
    const std::istreambuf_iterator<char> end;
    const std::string preservedContents(begin, end);
    const bool outputPreserved = outputOpened &&
        !output.bad() &&
        preservedContents == existingContents;

    std::remove(compressedPath.c_str());
    std::remove(outputPath.c_str());

    if (!decompressionRejected || !outputPreserved)
    {
        std::cerr << "FAIL: Existing output was not safely preserved.\n";
        return 1;
    }

    std::cout << "PASS: Existing output was protected.\n";
    return 0;
}
