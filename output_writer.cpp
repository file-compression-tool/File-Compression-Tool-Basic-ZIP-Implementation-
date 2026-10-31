#include "output_writer.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

namespace
{
    bool getFileSize(const std::string& path, std::uint64_t& size)
    {
        std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
        if (!file.is_open())
            return false;

        const std::streamoff end = file.tellg();
        if (end < 0)
            return false;

        size = static_cast<std::uint64_t>(end);
        return true;
    }
}

bool writeOutputFile(const std::vector<std::uint8_t>& data,
                     const std::string& outputPath,
                     bool overwriteExisting)
{
    if (outputPath.empty())
    {
        std::cerr << "Error: Output path cannot be empty." << std::endl;
        return false;
    }

    if (!overwriteExisting)
    {
        std::ifstream existing(outputPath.c_str(), std::ios::binary);
        if (existing.is_open())
        {
            std::cerr << "Error: Output file already exists: " << outputPath
                      << std::endl;
            return false;
        }
    }

    std::ofstream output(outputPath.c_str(),
                         std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
        std::cerr << "Error: Could not open output file (check that the "
                     "folder exists and is writable): "
                  << outputPath << std::endl;
        return false;
    }

    if (!data.empty())
    {
        output.write(reinterpret_cast<const char*>(data.data()),
                     static_cast<std::streamsize>(data.size()));
    }
    output.flush();
    output.close();

    if (!output)
    {
        std::cerr << "Error: Failed while writing output file: "
                  << outputPath << std::endl;
        std::remove(outputPath.c_str());
        return false;
    }

    std::uint64_t writtenSize = 0;
    if (!getFileSize(outputPath, writtenSize) || writtenSize != data.size())
    {
        std::cerr << "Error: Output file size does not match the decoded "
                     "data: "
                  << outputPath << std::endl;
        std::remove(outputPath.c_str());
        return false;
    }

    return true;
}

bool filesAreIdentical(const std::string& pathA, const std::string& pathB)
{
    std::ifstream a(pathA.c_str(), std::ios::binary);
    if (!a.is_open())
    {
        std::cerr << "Error: Could not open file: " << pathA << std::endl;
        return false;
    }

    std::ifstream b(pathB.c_str(), std::ios::binary);
    if (!b.is_open())
    {
        std::cerr << "Error: Could not open file: " << pathB << std::endl;
        return false;
    }

    std::uint64_t sizeA = 0;
    std::uint64_t sizeB = 0;
    if (!getFileSize(pathA, sizeA) || !getFileSize(pathB, sizeB))
    {
        std::cerr << "Error: Could not determine file size." << std::endl;
        return false;
    }

    if (sizeA != sizeB)
    {
        std::cerr << "Mismatch: sizes differ (" << sizeA << " vs " << sizeB
                  << " bytes)." << std::endl;
        return false;
    }

    const std::size_t chunkSize = 65536;
    std::vector<char> bufferA(chunkSize);
    std::vector<char> bufferB(chunkSize);
    std::uint64_t offset = 0;

    while (a && b)
    {
        a.read(&bufferA[0], static_cast<std::streamsize>(chunkSize));
        b.read(&bufferB[0], static_cast<std::streamsize>(chunkSize));

        const std::streamsize readA = a.gcount();
        const std::streamsize readB = b.gcount();
        if (readA != readB)
        {
            std::cerr << "Mismatch: read lengths differ near byte " << offset
                      << "." << std::endl;
            return false;
        }

        for (std::streamsize i = 0; i < readA; ++i)
        {
            if (bufferA[static_cast<std::size_t>(i)] !=
                bufferB[static_cast<std::size_t>(i)])
            {
                std::cerr << "Mismatch: first difference at byte offset "
                          << (offset + static_cast<std::uint64_t>(i)) << "."
                          << std::endl;
                return false;
            }
        }
        offset += static_cast<std::uint64_t>(readA);

        if (readA == 0)
            break;
    }

    if (a.bad() || b.bad())
    {
        std::cerr << "Error: Read failure while comparing files."
                  << std::endl;
        return false;
    }

    return true;
}
