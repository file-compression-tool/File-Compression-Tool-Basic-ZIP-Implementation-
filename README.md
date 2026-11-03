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

## Continuous integration and releases

GitHub Actions builds the application and runs the CLI tests on pushes and pull
requests targeting `main` or `master`. To publish a release, push a version tag
such as:

```sh
git tag v1.0.0
git push origin v1.0.0
```

The workflow reruns the build and tests, then creates a GitHub Release with
automatically generated release notes and attaches the Linux `file-compressor`
executable. The release is published only if the tests pass.
