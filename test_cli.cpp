#include "cli.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    bool expect(bool condition, const std::string& description)
    {
        if (!condition)
            std::cerr << "FAILED: " << description << std::endl;
        return condition;
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
            std::vector<std::string>{"compress", "input.txt", "--unknown"},
            options,
            errorMessage);
        if (!expect(rejectedUnknownOption, "reject unknown option"))
            return false;

        const bool rejectedDecompression = !parseCliArguments(
            std::vector<std::string>{"decompress", "input.huff"},
            options,
            errorMessage);
        return expect(rejectedDecompression &&
                          errorMessage.find("not available") !=
                              std::string::npos,
                      "report decompression as unavailable");
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
                            helpText.find("Decompression is not available") !=
                                std::string::npos,
                        "describe available options and unsupported decoding"))
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

        std::ifstream preservedFile(outputPath.c_str(), std::ios::binary);
        std::string preserved;
        preserved.assign(
            std::istreambuf_iterator<char>(preservedFile),
            std::istreambuf_iterator<char>());
        preservedFile.close();

        if (!expect(rejected != 0, "cancel unconfirmed overwrite"))
            return false;
        if (!expect(preserved == "keep",
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
}

int main()
{
    if (!testArgumentParsing() || !testHelpAndUsage() ||
        !testCompressionCommand() ||
        !testEmptyInputStatistics() || !testOverwriteConfirmation() ||
        !testForceOverwrite())
        return 1;

    std::cout << "All CLI tests passed." << std::endl;
    return 0;
}
