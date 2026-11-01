
#include "cli.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>
#include <vector>

namespace
{
    bool expect(bool condition, const std::string& description)
    {
        if (!condition)
            std::cerr << "FAILED: " << description << std::endl;

        return condition;
    }

    bool readFileContents(
        const std::string& path,
        std::string& contents)
    {
        std::ifstream file(path.c_str(), std::ios::binary);

        if (!file.is_open())
            return false;

        contents.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());

        return !file.bad();
    }

    bool createCompressedFixture(
        const std::string& inputPath,
        const std::string& compressedPath,
        const std::string& original)
    {
        std::remove(inputPath.c_str());
        std::remove(compressedPath.c_str());

        {
            std::ofstream file(inputPath.c_str(), std::ios::binary);

            if (!file.is_open())
                return false;

            if (!original.empty())
            {
                file.write(
                    original.data(),
                    static_cast<std::streamsize>(original.size()));
            }

            if (!file)
                return false;
        }

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream input;

        const int result = runCli(
            std::vector<std::string>{
                "compress", inputPath, "-o", compressedPath
            },
            input,
            output,
            errors);

        return result == 0 && errors.str().empty();
    }

    bool testArgumentParsing()
    {
        CliOptions options;
        std::string errorMessage;

        const bool explicitOutput = parseCliArguments(
            std::vector<std::string>{
                "compress", "input.txt", "--output", "result.huff"
            },
            options,
            errorMessage);

        if (!expect(explicitOutput, "parse explicit output path"))
            return false;

        if (!expect(options.command == CliCommand::Compress &&
                        options.inputPath == "input.txt" &&
                        options.outputPath == "result.huff",
                    "store parsed compression arguments"))
            return false;

        const bool defaultOutput = parseCliArguments(
            std::vector<std::string>{"compress", "input.txt"},
            options,
            errorMessage);

        if (!expect(defaultOutput, "parse default output path"))
            return false;

        if (!expect(options.outputPath == "input.txt.huff",
                    "derive default output path"))
            return false;

        const bool parsedForce = parseCliArguments(
            std::vector<std::string>{
                "compress", "input.txt", "--force"
            },
            options,
            errorMessage);

        if (!expect(parsedForce && options.forceOverwrite,
                    "parse deliberate overwrite option"))
            return false;

        const bool rejectedMissingInput = !parseCliArguments(
            std::vector<std::string>{"compress"},
            options,
            errorMessage);

        if (!expect(rejectedMissingInput, "reject missing input path"))
            return false;

        const bool rejectedUnknownOption = !parseCliArguments(
            std::vector<std::string>{
                "compress", "input.txt", "--unknown"
            },
            options,
            errorMessage);

        if (!expect(rejectedUnknownOption, "reject unknown option"))
            return false;

        const bool acceptedDecompression = parseCliArguments(
            std::vector<std::string>{"decompress", "input.huff"},
            options,
            errorMessage);

        return expect(
            acceptedDecompression &&
            options.command == CliCommand::Decompress &&
            options.inputPath == "input.huff" &&
            options.outputPath == "input",
            "accept decompression command");
    }

    bool testHelpAndUsage()
    {
        const std::vector<std::vector<std::string> > helpArguments = {
            std::vector<std::string>(),
            std::vector<std::string>{"--help"},
            std::vector<std::string>{"-h"},
            std::vector<std::string>{"compress", "--help"},
            std::vector<std::string>{"compress", "input.txt", "-h"}
        };

        for (std::size_t i = 0; i < helpArguments.size(); ++i)
        {
            std::ostringstream output;
            std::ostringstream errors;
            std::istringstream input;

            const int result = runCli(
                helpArguments[i], input, output, errors);

            const std::string helpText = output.str();

            if (!expect(result == 0, "return success for help invocation"))
                return false;

            if (!expect(errors.str().empty(),
                        "write requested help to standard output"))
                return false;

            if (!expect(helpText.find("Usage:") != std::string::npos &&
                            helpText.find("Commands:") != std::string::npos &&
                            helpText.find("Options:") != std::string::npos &&
                            helpText.find("Examples:") != std::string::npos,
                        "include command, options, and examples in help"))
                return false;

            if (!expect(helpText.find("--output") != std::string::npos &&
                            helpText.find("--force") != std::string::npos &&
                            helpText.find("decompress") != std::string::npos,
                        "describe compression and decompression commands"))
                return false;
        }

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream input;

        const int result = runCli(
            std::vector<std::string>{"unknown"},
            input,
            output,
            errors);

        return expect(result != 0, "return failure for unknown command") &&
               expect(errors.str().find("Usage:") != std::string::npos,
                      "include usage after command errors");
    }

    bool testCompressionCommand()
    {
        const std::string inputPath = "cli_test_input.txt";
        const std::string outputPath = "cli_test_output.huff";

        std::remove(inputPath.c_str());
        std::remove(outputPath.c_str());

        {
            std::ofstream input(inputPath.c_str(), std::ios::binary);
            input << "CLI test input";

            if (!input)
                return expect(false, "create CLI input fixture");
        }

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream input;

        const int result = runCli(
            std::vector<std::string>{
                "compress", inputPath, "-o", outputPath
            },
            input,
            output,
            errors);

        std::ifstream compressed(outputPath.c_str(), std::ios::binary);

        char magic[4] = {};
        compressed.read(magic, sizeof(magic));

        const bool validOutput =
            compressed.gcount() == 4 &&
            magic[0] == 'H' && magic[1] == 'U' &&
            magic[2] == 'F' && magic[3] == 'F';

        compressed.close();

        const std::string outputText = output.str();

        std::remove(inputPath.c_str());
        std::remove(outputPath.c_str());

        return expect(result == 0, "compress through CLI") &&
               expect(validOutput, "create a valid compressed output") &&
               expect(outputText.find("Compression successful.") !=
                          std::string::npos,
                      "display successful command status") &&
               expect(outputText.find("Original size: 14 bytes") !=
                          std::string::npos,
                      "display original size") &&
               expect(outputText.find("Compressed size: ") !=
                          std::string::npos,
                      "display compressed size") &&
               expect(outputText.find(
                          "Compression ratio (original/compressed): ") !=
                          std::string::npos,
                      "display compression ratio") &&
               expect(errors.str().empty(), "do not report CLI errors");
    }

    bool testEmptyInputStatistics()
    {
        const std::string inputPath = "cli_test_empty_input";
        const std::string outputPath = "cli_test_empty_output.huff";

        std::remove(inputPath.c_str());
        std::remove(outputPath.c_str());

        {
            std::ofstream input(inputPath.c_str(), std::ios::binary);

            if (!input)
                return expect(false, "create empty CLI input fixture");
        }

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream input;

        const int result = runCli(
            std::vector<std::string>{
                "compress", inputPath, "-o", outputPath
            },
            input,
            output,
            errors);

        const std::string outputText = output.str();

        std::remove(inputPath.c_str());
        std::remove(outputPath.c_str());

        return expect(result == 0, "compress empty input through CLI") &&
               expect(outputText.find("Original size: 0 bytes") !=
                          std::string::npos,
                      "display empty original size") &&
               expect(outputText.find("Compressed size: 23 bytes") !=
                          std::string::npos,
                      "display empty-input header size") &&
               expect(outputText.find(
                          "Compression ratio: N/A (empty input)") !=
                          std::string::npos,
                      "avoid division by zero for empty input") &&
               expect(errors.str().empty(),
                      "do not report errors for empty input");
    }

    bool testOverwriteConfirmation()
    {
        const std::string inputPath = "cli_test_overwrite_input";
        const std::string outputPath = "cli_test_overwrite_output.huff";

        std::remove(inputPath.c_str());
        std::remove(outputPath.c_str());

        {
            std::ofstream input(inputPath.c_str(), std::ios::binary);
            input << "overwrite test";

            if (!input)
                return expect(false, "create overwrite input fixture");
        }

        {
            std::ofstream existing(outputPath.c_str(), std::ios::binary);
            existing << "keep";

            if (!existing)
                return expect(false, "create existing output fixture");
        }

        std::ostringstream rejectedOutput;
        std::ostringstream rejectedErrors;
        std::istringstream noAnswer("n\n");

        const int rejected = runCli(
            std::vector<std::string>{
                "compress", inputPath, "-o", outputPath
            },
            noAnswer,
            rejectedOutput,
            rejectedErrors);

        std::string preserved;
        const bool readPreserved =
            readFileContents(outputPath, preserved);

        if (!expect(rejected != 0, "cancel unconfirmed overwrite"))
            return false;

        if (!expect(readPreserved && preserved == "keep",
                    "preserve existing file after declining"))
            return false;

        if (!expect(rejectedOutput.str().find("Overwrite? [y/N]:") !=
                        std::string::npos,
                    "prompt before replacing existing output"))
            return false;

        if (!expect(rejectedErrors.str().find(
                        "existing output was not changed.") !=
                        std::string::npos,
                    "explain that declined overwrite preserved the file"))
            return false;

        std::ostringstream confirmedOutput;
        std::ostringstream confirmedErrors;
        std::istringstream yesAnswer("YES\n");

        const int confirmed = runCli(
            std::vector<std::string>{
                "compress", inputPath, "-o", outputPath
            },
            yesAnswer,
            confirmedOutput,
            confirmedErrors);

        std::ifstream replacedFile(outputPath.c_str(), std::ios::binary);

        char magic[4] = {};
        replacedFile.read(magic, sizeof(magic));

        const bool replaced =
            replacedFile.gcount() == 4 &&
            magic[0] == 'H' && magic[1] == 'U' &&
            magic[2] == 'F' && magic[3] == 'F';

        replacedFile.close();

        std::remove(inputPath.c_str());
        std::remove(outputPath.c_str());

        return expect(confirmed == 0, "allow confirmed overwrite") &&
               expect(replaced, "replace only after confirmation") &&
               expect(confirmedErrors.str().empty(),
                      "do not report errors after confirmed overwrite");
    }

    bool testForceOverwrite()
    {
        const std::string inputPath = "cli_test_force_input";
        const std::string outputPath = "cli_test_force_output.huff";

        std::remove(inputPath.c_str());
        std::remove(outputPath.c_str());

        {
            std::ofstream input(inputPath.c_str(), std::ios::binary);
            input << "force test";

            if (!input)
                return expect(false, "create force input fixture");
        }

        {
            std::ofstream existing(outputPath.c_str(), std::ios::binary);
            existing << "keep";

            if (!existing)
                return expect(false, "create force output fixture");
        }

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream noInput;

        const int result = runCli(
            std::vector<std::string>{
                "compress", inputPath, "-o", outputPath, "--force"
            },
            noInput,
            output,
            errors);

        const std::string outputText = output.str();

        std::ifstream replacedFile(outputPath.c_str(), std::ios::binary);

        char magic[4] = {};
        replacedFile.read(magic, sizeof(magic));

        const bool replaced =
            replacedFile.gcount() == 4 &&
            magic[0] == 'H' && magic[1] == 'U' &&
            magic[2] == 'F' && magic[3] == 'F';

        replacedFile.close();

        std::remove(inputPath.c_str());
        std::remove(outputPath.c_str());

        return expect(result == 0, "allow deliberate --force overwrite") &&
               expect(replaced, "force writes a valid compressed file") &&
               expect(outputText.find("Overwrite? [y/N]:") ==
                          std::string::npos,
                      "skip prompt with --force") &&
               expect(errors.str().empty(),
                      "do not report errors after forced overwrite");
    }

    bool testDecompressionCommand()
    {
        const std::string inputPath = "cli_decompress_input.txt";
        const std::string compressedPath = "cli_decompress_input.huff";
        const std::string restoredPath = "cli_decompress_restored.txt";

        std::remove(inputPath.c_str());
        std::remove(compressedPath.c_str());
        std::remove(restoredPath.c_str());

        const std::string original = "Huffman CLI decompression test";

        {
            std::ofstream file(inputPath.c_str(), std::ios::binary);

            file.write(
                original.data(),
                static_cast<std::streamsize>(original.size()));

            if (!file)
                return expect(false, "create decompression test input");
        }

        std::ostringstream compressOutput;
        std::ostringstream compressErrors;
        std::istringstream compressInput;

        const int compressResult = runCli(
            std::vector<std::string>{
                "compress", inputPath, "-o", compressedPath
            },
            compressInput,
            compressOutput,
            compressErrors);

        if (!expect(compressResult == 0,
                    "prepare compressed file for decompression test"))
        {
            std::remove(inputPath.c_str());
            std::remove(compressedPath.c_str());
            return false;
        }

        std::ostringstream decompressOutput;
        std::ostringstream decompressErrors;
        std::istringstream decompressInput;

        const int decompressResult = runCli(
            std::vector<std::string>{
                "decompress", compressedPath, "-o", restoredPath
            },
            decompressInput,
            decompressOutput,
            decompressErrors);

        std::string restored;
        const bool readRestored =
            readFileContents(restoredPath, restored);

        const bool success =
            expect(decompressResult == 0,
                   "decompress through CLI") &&
            expect(readRestored && restored == original,
                   "restore exact original content") &&
            expect(decompressOutput.str().find(
                       "Decompression successful.") != std::string::npos,
                   "display decompression success") &&
            expect(decompressErrors.str().empty(),
                   "do not report decompression errors");

        std::remove(inputPath.c_str());
        std::remove(compressedPath.c_str());
        std::remove(restoredPath.c_str());

        return success;
    }

    // Test decompression of an empty file.
    bool testEmptyFileDecompression()
    {
        const std::string inputPath = "cli_empty_input";
        const std::string compressedPath = "cli_empty_input.huff";
        const std::string restoredPath = "cli_empty_restored";

        const bool compressed =
            createCompressedFixture(inputPath, compressedPath, "");

        if (!expect(compressed, "prepare empty compressed file"))
            return false;

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream input;

        const int result = runCli(
            std::vector<std::string>{
                "decompress", compressedPath, "-o", restoredPath
            },
            input,
            output,
            errors);

        std::string restored;
        const bool readRestored =
            readFileContents(restoredPath, restored);

        const bool success =
            expect(result == 0, "decompress empty file") &&
            expect(readRestored && restored.empty(),
                   "restore an empty output file") &&
            expect(output.str().find("Decompressed size: 0 bytes") !=
                       std::string::npos,
                   "report zero decompressed bytes") &&
            expect(errors.str().empty(),
                   "do not report errors for empty decompression");

        std::remove(inputPath.c_str());
        std::remove(compressedPath.c_str());
        std::remove(restoredPath.c_str());

        return success;
    }

    // Test all possible byte values through the CLI.
    bool testBinaryFileRoundTrip()
    {
        const std::string inputPath = "cli_binary_input";
        const std::string compressedPath = "cli_binary_input.huff";
        const std::string restoredPath = "cli_binary_restored";

        std::string original;

        for (int value = 0; value < 256; ++value)
            original.push_back(static_cast<char>(value));

        const bool compressed =
            createCompressedFixture(inputPath, compressedPath, original);

        if (!expect(compressed, "prepare binary compressed file"))
            return false;

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream input;

        const int result = runCli(
            std::vector<std::string>{
                "decompress", compressedPath, "-o", restoredPath
            },
            input,
            output,
            errors);

        std::string restored;
        const bool readRestored =
            readFileContents(restoredPath, restored);

        const bool success =
            expect(result == 0, "decompress binary file through CLI") &&
            expect(readRestored && restored == original,
                   "restore all 256 byte values exactly") &&
            expect(errors.str().empty(),
                   "do not report errors for binary decompression");

        std::remove(inputPath.c_str());
        std::remove(compressedPath.c_str());
        std::remove(restoredPath.c_str());

        return success;
    }

    // Test rejection of an invalid compressed file.
    bool testInvalidCompressedFile()
    {
        const std::string invalidPath = "cli_invalid_input.huff";
        const std::string outputPath = "cli_invalid_restored";

        std::remove(invalidPath.c_str());
        std::remove(outputPath.c_str());

        {
            std::ofstream file(invalidPath.c_str(), std::ios::binary);

            file << "This is not a valid HUFF file";

            if (!file)
                return expect(false, "create invalid compressed fixture");
        }

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream input;

        const int result = runCli(
            std::vector<std::string>{
                "decompress", invalidPath, "-o", outputPath
            },
            input,
            output,
            errors);

        std::ifstream restored(outputPath.c_str(), std::ios::binary);
        const bool outputWasNotCreated = !restored.is_open();

        const bool success =
            expect(result != 0, "reject invalid compressed file") &&
            expect(errors.str().find("Invalid or unreadable compressed file") !=
                       std::string::npos,
                   "report invalid compressed input") &&
            expect(outputWasNotCreated,
                   "do not create output for invalid compressed input");

        std::remove(invalidPath.c_str());
        std::remove(outputPath.c_str());

        return success;
    }

    // Test that declining decompression overwrite preserves existing data.
    bool testDecompressionOverwriteDeclined()
    {
        const std::string inputPath = "cli_overwrite_input";
        const std::string compressedPath = "cli_overwrite_input.huff";
        const std::string restoredPath = "cli_overwrite_restored";

        const std::string original = "New decompressed content";

        const bool compressed =
            createCompressedFixture(inputPath, compressedPath, original);

        if (!expect(compressed, "prepare overwrite compressed fixture"))
            return false;

        {
            std::ofstream existing(restoredPath.c_str(), std::ios::binary);
            existing << "keep existing content";

            if (!existing)
                return expect(false, "create existing decompression output");
        }

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream input("n\n");

        const int result = runCli(
            std::vector<std::string>{
                "decompress", compressedPath, "-o", restoredPath
            },
            input,
            output,
            errors);

        std::string preserved;
        const bool readPreserved =
            readFileContents(restoredPath, preserved);

        const bool success =
            expect(result != 0, "cancel decompression overwrite") &&
            expect(readPreserved &&
                       preserved == "keep existing content",
                   "preserve existing output after declining") &&
            expect(output.str().find("Overwrite? [y/N]:") !=
                       std::string::npos,
                   "prompt before decompression overwrite") &&
            expect(errors.str().find(
                       "existing output was not changed.") !=
                       std::string::npos,
                   "explain declined decompression overwrite");

        std::remove(inputPath.c_str());
        std::remove(compressedPath.c_str());
        std::remove(restoredPath.c_str());

        return success;
    }

    // Test that --force replaces an existing decompression output.
    bool testDecompressionForceOverwrite()
    {
        const std::string inputPath = "cli_force_input";
        const std::string compressedPath = "cli_force_input.huff";
        const std::string restoredPath = "cli_force_restored";

        const std::string original = "Forced decompression content";

        const bool compressed =
            createCompressedFixture(inputPath, compressedPath, original);

        if (!expect(compressed, "prepare force decompression fixture"))
            return false;

        {
            std::ofstream existing(restoredPath.c_str(), std::ios::binary);
            existing << "old content";

            if (!existing)
                return expect(false, "create force decompression output");
        }

        std::ostringstream output;
        std::ostringstream errors;
        std::istringstream input;

        const int result = runCli(
            std::vector<std::string>{
                "decompress", compressedPath,
                "-o", restoredPath, "--force"
            },
            input,
            output,
            errors);

        std::string restored;
        const bool readRestored =
            readFileContents(restoredPath, restored);

        const bool success =
            expect(result == 0, "allow forced decompression overwrite") &&
            expect(readRestored && restored == original,
                   "replace existing output with decoded content") &&
            expect(output.str().find("Overwrite? [y/N]:") ==
                       std::string::npos,
                   "skip overwrite prompt with decompression --force") &&
            expect(errors.str().empty(),
                   "do not report errors after forced decompression");

        std::remove(inputPath.c_str());
        std::remove(compressedPath.c_str());
        std::remove(restoredPath.c_str());

        return success;
    }
}

int main()
{
    if (!testArgumentParsing() ||
        !testHelpAndUsage() ||
        !testCompressionCommand() ||
        !testEmptyInputStatistics() ||
        !testOverwriteConfirmation() ||
        !testForceOverwrite() ||
        !testDecompressionCommand() ||
        !testEmptyFileDecompression() ||
        !testBinaryFileRoundTrip() ||
        !testInvalidCompressedFile() ||
        !testDecompressionOverwriteDeclined() ||
        !testDecompressionForceOverwrite())
    {
        return 1;
    }

    std::cout << "All CLI tests passed." << std::endl;

    return 0;
}
