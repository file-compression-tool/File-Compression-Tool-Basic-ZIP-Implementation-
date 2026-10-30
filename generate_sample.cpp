#include "encoder.hpp"
#include "frequency.hpp"
#include "huffman_tree.hpp"
#include "file_reader.hpp"

#include <iostream>
#include <unordered_map>

int main()
{
    std::vector<std::uint8_t> data;

    if (!readInputFile("single.txt", data))
    {
        std::cerr << "Could not read single.txt\n";
        return 1;
    }

    auto frequencies = calculateFrequency(data);
    auto root = buildHuffmanTree(frequencies);
    auto codes = generateHuffmanCodes(root);

    std::unordered_map<std::uint8_t, std::string> encoderCodes(
        codes.begin(), codes.end()
    );

    if (!writeHuffFile(data, encoderCodes, "single.huff", true))
    {
        std::cerr << "Could not create single.huff\n";
        return 1;
    }

    std::cout << "Successfully created single.huff\n";
    return 0;
}