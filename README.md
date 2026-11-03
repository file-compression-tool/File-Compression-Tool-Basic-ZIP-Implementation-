
## Tests

Build and run the CLI tests from the repository root:

```sh
g++ -std=c++11 test_cli.cpp cli.cpp file_reader.cpp frequency.cpp huffman_tree.cpp encoder.cpp decoder.cpp -o test_cli
./test_cli
```

On Windows, run `.\test_cli.exe` after building the test executable with `-o test_cli.exe`.

Build and run the decompressor tests:

```sh
g++ -std=c++11 test_decompressor.cpp decompressor.cpp -o test_decompressor
./test_decompressor
```

## Continuous Integration and Releases

GitHub Actions builds the application and runs the CLI tests on pushes and pull requests targeting `main` or `master`.

To publish a release, push a version tag such as:

```sh
git tag v1.0.0
git push origin v1.0.0
```

The workflow reruns the build and tests, then creates a GitHub Release with automatically generated release notes and attaches the Linux `file-compressor` executable. The release is published only if the tests pass.
