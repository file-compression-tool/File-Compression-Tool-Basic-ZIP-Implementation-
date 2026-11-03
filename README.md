# File Compression Tool

This command-line tool compresses and decompresses files using Huffman coding
and the project's version 1 `.huff` format.

## Build

From the repository root, build with a C++11-compatible compiler:

```sh
g++ -std=c++11 main.cpp cli.cpp file_reader.cpp frequency.cpp huffman_tree.cpp encoder.cpp decoder.cpp -o file-compressor
```

On Windows, name the executable `file-compressor.exe` and run it as
`.\file-compressor.exe`.

## Usage

```text
file-compressor compress <input-file> [-o|--output <output-file>] [--force]
file-compressor decompress <input-file> [-o|--output <output-file>] [--force]
file-compressor -h | --help
```

Examples:

```sh
file-compressor compress report.txt
file-compressor compress report.txt --output archive.huff
file-compressor decompress archive.huff
file-compressor decompress archive.huff -o restored.txt
file-compressor --help
```

Run `file-compressor --help` or `file-compressor -h` for command descriptions,
options, and more examples. Help is also available after a command, for example
`file-compressor compress --help`.


## How It Works

The tool uses Huffman coding to compress files.

- **Compression:** Reads the input file, calculates character frequencies, builds a Huffman tree, and encodes the data into a `.huff` file.
- **Decompression:** Reads the `.huff` file, reconstructs the Huffman tree using the stored frequency information, and restores the original file.
- **Validation:** Checks the compressed file header, metadata, encoded data, and output size before accepting the decompressed result.


## Compressed File Format

The project uses a custom version 1 `.huff` format.

The file contains:

1. Magic signature (`HUFF`) to identify compressed files.
2. Format version.
3. Original file size.
4. Encoded bit count.
5. Symbol frequency table.
6. Huffman-encoded payload.

This metadata allows the decompressor to reconstruct the Huffman tree and restore the original content.


## Project Features

- File compression using Huffman coding.
- File decompression with original content restoration.
- Command-line interface for easy operation.
- Automatic output filename generation.
- Custom output path using `-o` or `--output`.
- Overwrite protection to prevent accidental data loss.
- Force overwrite option using `--force`.
- Input file validation and error handling.
- Validation of compressed file metadata and payload.
- Support for empty files and single-character files.


## Error Handling

The application handles different error conditions to ensure reliable operation.

| Error Condition | Expected Behaviour |
|---|---|
| Input file not found | Displays an error message |
| Input file cannot be read | Reports a read error |
| Invalid command | Displays usage information |
| Invalid option | Reports an argument error |
| Invalid `.huff` file | Rejects the compressed file |
| Existing output file | Requests overwrite confirmation |
| Output write failure | Reports an error |


## Supported Input Cases

The application handles different types of input files:

- Empty files.
- Text files.
- Files containing repeated characters.
- Files containing different character frequencies.
- Files containing binary data.

The decompression process validates the encoded data before restoring the original file.


## Output paths and overwrite protection

The default compressed output path is `<input-file>.huff`. For decompression,
the default output removes a trailing `.huff` extension; otherwise it appends
`.out`. Use `-o` or `--output` to choose another output path.

If the output already exists, the CLI asks for confirmation. Enter `y` or
`yes` to replace it; any other answer cancels and preserves the existing file.
Use `--force` to deliberately overwrite without prompting.

## Results and errors

After successful compression, the CLI reports the original size, compressed
size, and original-to-compressed ratio. The ratio is `N/A` for an empty input.
Successful decompression reports the restored size and output path.

Missing or unreadable inputs, invalid compressed files, unknown commands,
invalid options, and output write failures report an error and return a
non-zero exit status. Argument errors also print usage information.

## Tests

Build and run the CLI tests from the repository root:

```sh
g++ -std=c++11 test_cli.cpp cli.cpp file_reader.cpp frequency.cpp huffman_tree.cpp encoder.cpp decoder.cpp -o test_cli
./test_cli
```

On Windows, run `.\test_cli.exe` after building the test executable with
`-o test_cli.exe`.

g++ -std=c++11 test_decompressor.cpp decompressor.cpp -o test_decompressor
./test_decompressor
