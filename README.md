# File Compression Tool

This command-line tool compresses files into the project's version 1 `.huff`
format using Huffman coding.

## Build

From the repository root, build with a C++11-compatible compiler:

```sh
g++ -std=c++11 main.cpp cli.cpp file_reader.cpp frequency.cpp huffman_tree.cpp encoder.cpp -o file-compressor
```

On Windows, the resulting executable can be named `file-compressor.exe`.

## Usage

```text
file-compressor compress <input> [-o|--output <path>] [--force]
file-compressor --help
```

Run `file-compressor --help` (or `file-compressor -h`) for the complete
command list, option descriptions, and examples. Help is also available for a
specific command, for example `file-compressor compress --help`.

If `-o` / `--output` is omitted, the output path is `<input>.huff`. If that
output already exists, the CLI prompts before replacing it. Answer `y` or
`yes` to confirm; any other answer cancels and preserves the existing file. Use
`--force` to deliberately overwrite without a prompt.
On successful compression, the CLI displays the original size, compressed
size, and the original-to-compressed ratio. The ratio is reported as `N/A` for
an empty input.

Decompression is not available yet. It depends on implementation of
FCT-F-010 through FCT-F-014.
