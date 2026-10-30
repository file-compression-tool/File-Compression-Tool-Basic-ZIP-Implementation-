#include "cli.hpp"

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
                          const std::string& path)
    {
        output << "Output file already exists: " << path << std::endl
               << "Overwrite? [y/N]: " << std::flush;

        std::string answer;
        if (!std::getline(input, answer))
        {
            errors << "Error: Compression cancelled; existing output was "
                      "not changed."
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

        errors << "Error: Compression cancelled; existing output was not "
                  "changed."
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
                                      options.outputPath))
                {
                    return 1;
                }
                overwriteExisting = true;
            }

            if (!writeHuffFile(data,
                               codes,
                               options.outputPath,
                               overwriteExisting))
                return 1;
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
                   << std::fixed << std::setprecision(2) << ratio << ":1"
                   << std::endl;
        }
        return 0;
    }
}

bool parseCliArguments(const std::vector<std::string>& arguments,
                       CliOptions& options,
                       std::string& errorMessage)
{
    options = CliOptions();
    errorMessage.clear();

    if (arguments.empty() || arguments[0] == "--help" ||
        arguments[0] == "-h")
    {
        options.command = CliCommand::Help;
        return true;
    }

    if (arguments[0] != "compress")
    {
        if (arguments[0] == "decompress")
        {
            errorMessage =
                "Decompression is not available yet; it depends on "
                "FCT-F-010 through FCT-F-014.";
        }
        else
        {
            errorMessage = "Unknown command: " + arguments[0];
        }
        return false;
    }

    options.command = CliCommand::Compress;
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
        errorMessage = "The compress command requires an input file.";
        return false;
    }
    if (positionalArguments.size() > 1)
    {
        errorMessage = "Specify exactly one input file.";
        return false;
    }

    options.inputPath = positionalArguments[0];
    if (!outputPathProvided)
        options.outputPath = options.inputPath + ".huff";

    return true;
}

void printCliUsage(std::ostream& output)
{
    output << "Usage:\n"
           << "  file-compressor compress <input-file> "
              "[-o|--output <output-file>] [--force]\n"
           << "  file-compressor -h | --help\n\n"
           << "Commands:\n"
           << "  compress <input-file>  Compress a file using Huffman coding.\n\n"
           << "Options:\n"
           << "  -o, --output <path>    Set the output path. By default, "
              "<input-file>.huff is used.\n"
           << "  --force                Overwrite an existing output without "
              "prompting.\n"
           << "  -h, --help             Show this help message. May be used "
              "with or without a command.\n\n"
           << "Examples:\n"
           << "  file-compressor compress report.txt\n"
           << "  file-compressor compress report.txt -o archive.huff\n"
           << "  file-compressor compress report.txt --force\n\n"
           << "If the output already exists, the program asks for "
              "confirmation unless --force is used.\n"
           << "Decompression is not available in this version."
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

    return runCompression(options, input, output, errors);
}
