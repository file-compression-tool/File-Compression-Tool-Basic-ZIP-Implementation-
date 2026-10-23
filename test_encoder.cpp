#include "encoder.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
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

    std::vector<uint8_t> readBytes(const std::string& path)
    {
        std::ifstream input(path.c_str(), std::ios::binary);
        return std::vector<uint8_t>(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }

    bool testBitPackingAndHeader()
    {
        const std::string path = "encoder_test.huff";
        const std::vector<uint8_t> input = {'A', 'B', 'A'};
        const std::unordered_map<uint8_t, std::string> codes = {
            {'A', "0"}, {'B', "11"}
        };

        if (!expect(writeHuffFile(input, codes, path),
                    "write valid input"))
            return false;

        const std::vector<uint8_t> output = readBytes(path);
        std::remove(path.c_str());

        const std::vector<uint8_t> expected = {
            'H', 'U', 'F', 'F', 1,
            3, 0, 0, 0, 0, 0, 0, 0,
            4, 0, 0, 0, 0, 0, 0, 0,
            2, 0,
            'A', 2, 0, 0, 0, 0, 0, 0, 0,
            'B', 1, 0, 0, 0, 0, 0, 0, 0,
            0x60
        };
        return expect(output == expected,
                      "header metadata and MSB-first padded bits match format");
    }

    bool testEmptyInput()
    {
        const std::string path = "encoder_empty_test.huff";
        const std::vector<uint8_t> input;
        const std::unordered_map<uint8_t, std::string> codes;

        const bool wrote = writeHuffFile(input, codes, path);
        const std::vector<uint8_t> output = readBytes(path);
        std::remove(path.c_str());
        const std::vector<uint8_t> expected = {
            'H', 'U', 'F', 'F', 1,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0
        };

        return expect(wrote, "write empty input") &&
               expect(output == expected, "empty header has no payload");
    }

    bool testHighBitSymbol()
    {
        const std::string path = "encoder_binary_test.huff";
        const std::vector<uint8_t> input = {
            static_cast<uint8_t>(0xff)
        };
        const std::unordered_map<uint8_t, std::string> codes = {
            {static_cast<uint8_t>(0xff), "1"}
        };

        const bool wrote = writeHuffFile(input, codes, path);
        const std::vector<uint8_t> output = readBytes(path);
        std::remove(path.c_str());

        return expect(wrote, "write high-bit input symbol") &&
               expect(output.size() == 33 &&
                          output[23] == static_cast<uint8_t>(0xff) &&
                          output[32] == static_cast<uint8_t>(0x80),
                      "preserve high-bit symbol and pack payload bit");
    }

    bool testInvalidCodeTables()
    {
        const std::string path = "encoder_invalid_test.huff";
        std::remove(path.c_str());

        const std::vector<uint8_t> input = {'X'};
        const std::unordered_map<uint8_t, std::string> missingCode;
        const std::unordered_map<uint8_t, std::string> invalidCode = {
            {'X', "2"}
        };
        const std::unordered_map<uint8_t, std::string> emptyCode = {
            {'X', ""}
        };

        const bool missingRejected =
            !writeHuffFile(input, missingCode, path);
        const bool invalidRejected =
            !writeHuffFile(input, invalidCode, path);
        const bool emptyRejected =
            !writeHuffFile(input, emptyCode, path);
        bool outputWasNotCreated = false;
        {
            const std::ifstream output(path.c_str(), std::ios::binary);
            outputWasNotCreated = !output.is_open();
        }
        std::remove(path.c_str());

        return expect(missingRejected, "reject missing code") &&
               expect(invalidRejected, "reject non-binary code") &&
               expect(emptyRejected,
                      "reject empty code for an occurring symbol") &&
               expect(outputWasNotCreated,
                      "reject invalid codes before creating output");
    }

    bool testUnwritableDestination()
    {
        const std::vector<uint8_t> input = {'X'};
        const std::unordered_map<uint8_t, std::string> codes = {
            {'X', "0"}
        };
        return expect(!writeHuffFile(input, codes,
                                     "missing_encoder_test_dir/out.huff"),
                      "report output open failure");
    }

    bool testOverwriteProtection()
    {
        const std::string path = "encoder_existing_test.huff";
        {
            std::ofstream existing(path.c_str(), std::ios::binary);
            existing.put('K');
        }

        const std::vector<uint8_t> input = {'X'};
        const std::unordered_map<uint8_t, std::string> codes = {
            {'X', "0"}
        };
        const bool rejected = !writeHuffFile(input, codes, path);
        const std::vector<uint8_t> preserved = readBytes(path);
        const bool overwritten = writeHuffFile(input, codes, path, true);
        const std::vector<uint8_t> replaced = readBytes(path);
        std::remove(path.c_str());

        return expect(rejected, "reject overwrite unless explicitly requested") &&
               expect(preserved == std::vector<uint8_t>(1, 'K'),
                      "preserve existing output after rejected overwrite") &&
               expect(overwritten, "allow explicitly requested overwrite") &&
               expect(replaced.size() == 33 && replaced[0] == 'H' &&
                          replaced[1] == 'U' && replaced[2] == 'F' &&
                          replaced[3] == 'F',
                      "replace destination with a valid .huff file");
    }
}

int main()
{
    const bool passed =
        testBitPackingAndHeader() &&
        testEmptyInput() &&
        testHighBitSymbol() &&
        testInvalidCodeTables() &&
        testUnwritableDestination() &&
        testOverwriteProtection();

    if (!passed)
        return 1;

    std::cout << "All encoder tests passed." << std::endl;
    return 0;
}
