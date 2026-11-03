
#include "huffman_tree.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <limits>

bool isPrefixFree(const HuffmanCodeMap& codes)
{
    for (auto first = codes.begin(); first != codes.end(); ++first)
    {
        for (auto second = codes.begin(); second != codes.end(); ++second)
        {
            if (first == second)
                continue;

            const std::string& a = first->second;
            const std::string& b = second->second;

            if (b.size() >= a.size() &&
                b.compare(0, a.size(), a) == 0)
            {
                return false;
            }
        }
    }

    return true;
}

void testEmptyFrequencyTable()
{
    std::map<uint8_t, std::uint64_t> frequencies;

    auto root = buildHuffmanTree(frequencies);

    assert(root == nullptr);
    assert(generateHuffmanCodes(root).empty());
}

void testSingleSymbol()
{
    std::map<uint8_t, std::uint64_t> frequencies = {
        {'A', 5}
    };

    auto root = buildHuffmanTree(frequencies);
    auto codes = generateHuffmanCodes(root);

    assert(root != nullptr);
    assert(root->isLeaf());
    assert(codes.size() == 1);
    assert(codes.at('A') == "0");
}

void testMultipleSymbols()
{
    std::map<uint8_t, std::uint64_t> frequencies = {
        {'A', 5},
        {'B', 2},
        {'C', 1},
        {'D', 1}
    };

    auto root = buildHuffmanTree(frequencies);
    auto codes = generateHuffmanCodes(root);

    assert(root != nullptr);
    assert(!root->isLeaf());
    assert(codes.size() == frequencies.size());
    assert(isPrefixFree(codes));

    for (const auto& entry : frequencies)
    {
        assert(codes.find(entry.first) != codes.end());
        assert(!codes.at(entry.first).empty());
    }
}

void testZeroFrequencyRejected()
{
    std::map<uint8_t, std::uint64_t> frequencies = {
        {'A', 0}
    };

    bool threw = false;

    try
    {
        buildHuffmanTree(frequencies);
    }
    catch (const std::invalid_argument&)
    {
        threw = true;
    }

    assert(threw);
}

void testNullRoot()
{
    std::shared_ptr<HuffmanNode> root = nullptr;

    auto codes = generateHuffmanCodes(root);

    assert(codes.empty());
}

void testEqualFrequencies()
{
    std::map<uint8_t, std::uint64_t> frequencies = {
        {'A', 5},
        {'B', 5},
        {'C', 5},
        {'D', 5}
    };

    auto root = buildHuffmanTree(frequencies);
    auto codes = generateHuffmanCodes(root);

    assert(root != nullptr);
    assert(codes.size() == frequencies.size());
    assert(isPrefixFree(codes));

    for (const auto& entry : frequencies)
    {
        assert(codes.find(entry.first) != codes.end());
        assert(!codes.at(entry.first).empty());
    }
}

void testDeterministicCodes()
{
    std::map<uint8_t, std::uint64_t> frequencies = {
        {'A', 5},
        {'B', 5},
        {'C', 3},
        {'D', 3}
    };

    auto root1 = buildHuffmanTree(frequencies);
    auto codes1 = generateHuffmanCodes(root1);

    auto root2 = buildHuffmanTree(frequencies);
    auto codes2 = generateHuffmanCodes(root2);

    assert(codes1 == codes2);
}

void testEverySymbolHasCode()
{
    std::map<uint8_t, std::uint64_t> frequencies = {
        {'A', 10},
        {'B', 7},
        {'C', 4},
        {'D', 2},
        {'E', 1}
    };

    auto root = buildHuffmanTree(frequencies);
    auto codes = generateHuffmanCodes(root);

    assert(root != nullptr);
    assert(codes.size() == frequencies.size());

    for (const auto& entry : frequencies)
    {
        assert(codes.count(entry.first) == 1);
        assert(!codes.at(entry.first).empty());
    }
}

void testMalformedTreeRejected()
{
    auto root = std::make_shared<HuffmanNode>('A', 10);
    auto child = std::make_shared<HuffmanNode>('B', 5);

    root->left = child;

    bool threw = false;

    try
    {
        generateHuffmanCodes(root);
    }
    catch (const std::invalid_argument&)
    {
        threw = true;
    }

    assert(threw);
}

// New test: frequency overflow must be rejected.
void testFrequencyOverflowRejected()
{
    std::map<uint8_t, std::uint64_t> frequencies = {
        {'A', std::numeric_limits<std::uint64_t>::max()},
        {'B', 1}
    };

    bool threw = false;

    try
    {
        buildHuffmanTree(frequencies);
    }
    catch (const std::overflow_error&)
    {
        threw = true;
    }

    assert(threw);
}

int main()
{
    std::cout << "Test program started" << std::endl;

    testEmptyFrequencyTable();
    std::cout << "Empty frequency test passed" << std::endl;

    testSingleSymbol();
    std::cout << "Single-symbol test passed" << std::endl;

    testMultipleSymbols();
    std::cout << "Multiple-symbol test passed" << std::endl;

    testZeroFrequencyRejected();
    std::cout << "Zero-frequency test passed" << std::endl;

    testNullRoot();
    std::cout << "Null-root test passed" << std::endl;

    testEqualFrequencies();
    std::cout << "Equal-frequency test passed" << std::endl;

    testDeterministicCodes();
    std::cout << "Deterministic-codes test passed" << std::endl;

    testEverySymbolHasCode();
    std::cout << "Every-symbol-code test passed" << std::endl;

    testMalformedTreeRejected();
    std::cout << "Malformed-tree test passed" << std::endl;

    testFrequencyOverflowRejected();
    std::cout << "Frequency-overflow test passed" << std::endl;

    std::cout << "All Huffman tree tests passed!" << std::endl;

    return 0;
}
