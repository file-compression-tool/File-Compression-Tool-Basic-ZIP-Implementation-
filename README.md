
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

## Automated Testing, Coverage and Static Analysis (CI/CD)

[![C++ CI](https://github.com/file-compression-tool/File-Compression-Tool-Basic-ZIP-Implementation-/actions/workflows/ci.yml/badge.svg)](https://github.com/file-compression-tool/File-Compression-Tool-Basic-ZIP-Implementation-/actions/workflows/ci.yml)

Every push and pull request runs the GitHub Actions pipeline in `.github/workflows/ci.yml`:

1. **Build and Test**: compiles the app and runs all unit, module and integration tests.
2. **Quality**: code coverage (fails below 70% line coverage), 18 end-to-end integration tests, and static analysis (cppcheck, clang-tidy, strict g++ warnings). Reports are attached to each run as the `quality-reports` artifact.
3. **Release**: for version tags only, and only if the checks above pass.

Run the same checks locally with `bash quality/run_coverage.sh`, `bash quality/run_integration_tests.sh` and `bash quality/run_static_analysis.sh`.

The workflow reruns the build and tests, then creates a GitHub Release with automatically generated release notes and attaches the Linux `file-compressor` executable. The release is published only if the tests pass.
