
#include "cli.hpp"

#include "decoder.hpp"
#include "encoder.hpp"
#include "file_reader.hpp"
#include "frequency.hpp"
#include "huffman_tree.hpp"

#include <cctype>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace
{
    bool startsWithDash(const std::string& value)
    {
        return !value.empty() && value[0] == '-';
    }

    bool outputExists(const std::string& path)
    {
        std::ifstream existing(path.c_str(), std::ios::binary);
        return existing.is_open();
    }

    bool confirmOverwrite(std::istream& input,
                          std::ostream& output,
                          std::ostream& errors,
                          const std::string& path,
                          const std::string& operation)
    {
        output << "Output file already exists: " << path << std::endl
               << "Overwrite? [y/N]: " << std::flush;

        std::string answer;

        if (!std::getline(input, answer))
        {
            errors << "Error: " << operation
                   << " cancelled; existing output was not changed."
                   << std::endl;
            return false;
        }

        std::string normalizedAnswer;

        for (char character : answer)
        {
            if (!std::isspace(static_cast<unsigned char>(character)))
            {
                normalizedAnswer.push_back(static_cast<char>(
                    std::tolower(static_cast<unsigned char>(character))));
            }
        }

        if (normalizedAnswer == "y" || normalizedAnswer == "yes")
            return true;

        errors << "Error: " << operation
               << " cancelled; existing output was not changed."
               << std::endl;

        return false;
    }

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

    int runCompression(const CliOptions& options,
                       std::istream& input,
                       std::ostream& output,
                       std::ostream& errors)
    {
        std::vector<std::uint8_t> data;

        if (!readInputFile(options.inputPath, data))
            return 1;

        try
        {
            const FrequencyTable frequencies = calculateFrequency(data);

            const std::shared_ptr<HuffmanNode> tree =
                buildHuffmanTree(frequencies);

            const HuffmanCodeMap generatedCodes =
                generateHuffmanCodes(tree);

            const std::unordered_map<std::uint8_t, std::string> codes(
                generatedCodes.begin(), generatedCodes.end());

            bool overwriteExisting = options.forceOverwrite;

            if (!overwriteExisting && outputExists(options.outputPath))
            {
                if (!confirmOverwrite(input,
                                      output,
                                      errors,
                                      options.outputPath,
                                      "Compression"))
                {
                    return 1;
                }

                overwriteExisting = true;
            }

            if (!writeHuffFile(data,
                               codes,
                               options.outputPath,
                               overwriteExisting))
            {
                return 1;
            }
        }
        catch (const std::exception& error)
        {
            errors << "Error: Compression failed: " << error.what()
                   << std::endl;
            return 1;
        }

        output << "Compression successful." << std::endl;
        output << "Original size: " << data.size() << " bytes" << std::endl;
        output << "Output: " << options.outputPath << std::endl;

        std::uint64_t compressedSize = 0;

        if (!getFileSize(options.outputPath, compressedSize))
        {
            output << "Compressed size: unavailable" << std::endl;
            output << "Compression ratio: unavailable" << std::endl;

            errors << "Warning: Compression succeeded, but the output size "
                      "could not be determined."
                   << std::endl;

            return 0;
        }

        output << "Compressed size: " << compressedSize << " bytes"
               << std::endl;

        if (data.empty())
        {
            output << "Compression ratio: N/A (empty input)" << std::endl;
        }
        else if (compressedSize == 0)
        {
            output << "Compression ratio: unavailable (empty output)"
                   << std::endl;
        }
        else
        {
            const long double ratio =
                static_cast<long double>(data.size()) /
                static_cast<long double>(compressedSize);

            output << "Compression ratio (original/compressed): "
                   << std::fixed << std::setprecision(2)
                   << ratio << ":1" << std::endl;
        }

        return 0;
    }

    int runDecompression(const CliOptions& options,
                         std::istream& input,
                         std::ostream& output,
                         std::ostream& errors)
    {
        std::vector<std::uint8_t> decodedData;

        if (!readHuffFile(options.inputPath, decodedData))
        {
            errors << "Error: Decompression failed. "
                      "Invalid or unreadable compressed file."
                   << std::endl;
            return 1;
        }

        bool overwriteExisting = options.forceOverwrite;

        if (!overwriteExisting && outputExists(options.outputPath))
        {
            if (!confirmOverwrite(input,
                                  output,
                                  errors,
                                  options.outputPath,
                                  "Decompression"))
            {
                return 1;
            }

            overwriteExisting = true;
        }

        std::ofstream outputFile(
            options.outputPath.c_str(),
            std::ios::binary | std::ios::trunc);

        if (!outputFile.is_open())
        {
            errors << "Error: Could not create output file: "
                   << options.outputPath << std::endl;
            return 1;
        }

        if (!decodedData.empty())
        {
            outputFile.write(
                reinterpret_cast<const char*>(decodedData.data()),
                static_cast<std::streamsize>(decodedData.size()));
        }

        if (!outputFile.good())
        {
            errors << "Error: Failed while writing decompressed data."
                   << std::endl;

            outputFile.close();
            std::remove(options.outputPath.c_str());

            return 1;
        }

        outputFile.close();

        if (outputFile.fail())
        {
            errors << "Error: Could not finish writing decompressed file."
                   << std::endl;

            std::remove(options.outputPath.c_str());
            return 1;
        }

        output << "Decompression successful." << std::endl;
        output << "Decompressed size: "
               << decodedData.size() << " bytes" << std::endl;
        output << "Output: " << options.outputPath << std::endl;

        return 0;
    }
}

bool parseCliArguments(const std::vector<std::string>& arguments,
                       CliOptions& options,
                       std::string& errorMessage)
{
    options = CliOptions();
    errorMessage.clear();

    if (arguments.empty() ||
        arguments[0] == "--help" ||
        arguments[0] == "-h")
    {
        options.command = CliCommand::Help;
        return true;
    }

    if (arguments[0] == "compress")
    {
        options.command = CliCommand::Compress;
    }
    else if (arguments[0] == "decompress")
    {
        options.command = CliCommand::Decompress;
    }
    else
    {
        errorMessage = "Unknown command: " + arguments[0];
        return false;
    }

    bool outputPathProvided = false;
    bool forceProvided = false;
    bool parseOptions = true;

    std::vector<std::string> positionalArguments;

    for (std::size_t i = 1; i < arguments.size(); ++i)
    {
        const std::string& argument = arguments[i];

        if (parseOptions && argument == "--")
        {
            parseOptions = false;
        }
        else if (parseOptions &&
                 (argument == "-o" || argument == "--output"))
        {
            if (outputPathProvided)
            {
                errorMessage = "Specify the output path only once.";
                return false;
            }

            if (i + 1 >= arguments.size())
            {
                errorMessage = "Missing path after " + argument + ".";
                return false;
            }

            options.outputPath = arguments[++i];

            if (options.outputPath.empty())
            {
                errorMessage = "The output path cannot be empty.";
                return false;
            }

            outputPathProvided = true;
        }
        else if (parseOptions &&
                 (argument == "-h" || argument == "--help"))
        {
            options = CliOptions();
            options.command = CliCommand::Help;
            return true;
        }
        else if (parseOptions && argument == "--force")
        {
            if (forceProvided)
            {
                errorMessage = "Specify --force only once.";
                return false;
            }

            options.forceOverwrite = true;
            forceProvided = true;
        }
        else if (parseOptions && startsWithDash(argument))
        {
            errorMessage = "Unknown option: " + argument;
            return false;
        }
        else
        {
            positionalArguments.push_back(argument);
        }
    }

    if (positionalArguments.empty())
    {
        errorMessage = options.command == CliCommand::Compress
            ? "The compress command requires an input file."
            : "The decompress command requires an input file.";

        return false;
    }

    if (positionalArguments.size() > 1)
    {
        errorMessage = "Specify exactly one input file.";
        return false;
    }

    options.inputPath = positionalArguments[0];

    if (!outputPathProvided)
    {
        if (options.command == CliCommand::Compress)
        {
            options.outputPath = options.inputPath + ".huff";
        }
        else
        {
            const std::string extension = ".huff";

            if (options.inputPath.size() >= extension.size() &&
                options.inputPath.compare(
                    options.inputPath.size() - extension.size(),
                    extension.size(),
                    extension) == 0)
            {
                options.outputPath = options.inputPath.substr(
                    0,
                    options.inputPath.size() - extension.size());
            }
            else
            {
                options.outputPath = options.inputPath + ".out";
            }
        }
    }

    return true;
}

void printCliUsage(std::ostream& output)
{
    output << "Usage:\n"
           << "  file-compressor compress <input-file> "
              "[-o|--output <output-file>] [--force]\n"
           << "  file-compressor decompress <input-file> "
              "[-o|--output <output-file>] [--force]\n"
           << "  file-compressor -h | --help\n\n"

           << "Commands:\n"
           << "  compress <input-file>    Compress a file using Huffman coding.\n"
           << "  decompress <input-file>  Decompress a .huff file.\n\n"

           << "Options:\n"
           << "  -o, --output <path>      Set the output path.\n"
           << "  --force                  Overwrite an existing output without "
              "prompting.\n"
           << "  -h, --help               Show this help message.\n\n"

           << "Examples:\n"
           << "  file-compressor compress report.txt\n"
           << "  file-compressor compress report.txt -o archive.huff\n"
           << "  file-compressor decompress archive.huff\n"
           << "  file-compressor decompress archive.huff -o restored.txt\n"
           << "  file-compressor decompress archive.huff --force\n\n"

           << "If the output already exists, the program asks for "
              "confirmation unless --force is used."
           << std::endl;
}

int runCli(const std::vector<std::string>& arguments,
           std::istream& input,
           std::ostream& output,
           std::ostream& errors)
{
    CliOptions options;
    std::string errorMessage;

    if (!parseCliArguments(arguments, options, errorMessage))
    {
        errors << "Error: " << errorMessage << std::endl;
        printCliUsage(errors);
        return 2;
    }

    if (options.command == CliCommand::Help)
    {
        printCliUsage(output);
        return 0;
    }

    if (options.command == CliCommand::Compress)
    {
        return runCompression(options, input, output, errors);
    }

    if (options.command == CliCommand::Decompress)
    {
        return runDecompression(options, input, output, errors);
    }

    return 2;
}
