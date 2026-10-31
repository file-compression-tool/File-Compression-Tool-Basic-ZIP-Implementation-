# Output Writer & Integrity Check — Test Cases

## Module
Output Writer & Integrity Check (`output_writer.hpp` / `output_writer.cpp`, `verify_integrity.cpp`)

## Requirements
- FCT-F-014: Write decompressed data to the requested output file without unintended modification (TC-DECOMP-04).
- FCT-NF-001: Decompressed output is byte-for-byte identical to the original (TC-NF-01).
- Supports FCT-F-016 / FCT-SR-003 (no accidental overwrite), FCT-SR-004 (invalid output path) and FCT-SR-002 (no false success on a failed write).

## Design

| Function | Purpose |
|---|---|
| `bool writeOutputFile(const std::vector<uint8_t>& data, const std::string& outputPath, bool overwriteExisting = false)` | Writes decoded bytes in binary mode. Refuses to replace an existing file unless `overwriteExisting` is true. Verifies the size on disk and removes a partial file on failure. |
| `bool filesAreIdentical(const std::string& pathA, const std::string& pathB)` | Streams both files in 64 KB chunks and reports the first differing byte offset or a size mismatch. |
| `verify-integrity <original> <restored>` | Command-line wrapper around `filesAreIdentical`. Exit code 0 = identical, 1 = different, 2 = usage error. |

The overwrite flag and the "Error: ..." messages on `stderr` follow the same pattern as `writeHuffFile` in `encoder.cpp`.

## Test Cases

| Test ID | Scenario | Expected Result |
|---|---|---|
| OUT-01 | Bytes include `0x0A`, `0x0D`, `0x00`, `0xFF`, `0x1A` | File on disk equals the input exactly (no newline translation) |
| OUT-02 | All 256 byte values | All bytes preserved |
| OUT-03 | Empty data | Empty file created |
| OUT-04 | 1 MB of data | Written correctly |
| OUT-05 | Output exists, overwrite not allowed | Returns `false`; existing file unchanged |
| OUT-06 | Output exists, overwrite allowed | File fully replaced (no leftover old bytes) |
| OUT-07 | Output folder does not exist | Returns `false`; nothing created |
| OUT-08 | Empty output path | Returns `false` |
| INT-01 | Two identical files (> 64 KB) | `true` |
| INT-02 | One byte differs in the second chunk | `false` |
| INT-03 | Files differ in length | `false` |
| INT-04 | LF vs CRLF line ending | `false` |
| INT-05 | Both files empty | `true` |
| INT-06 | A file is missing | `false`, no crash |

## Full round-trip check (TC-NF-01)
Run once the `decompress` command is merged (FCT-F-010 to FCT-F-013). For each file in the corpus
(empty file, single repeated byte, text with CRLF line endings, binary with all 256 byte values, large file):

```text
file-compressor compress sample.bin -o sample.huff
file-compressor decompress sample.huff -o restored.bin
verify-integrity sample.bin restored.bin
```

Expected result: `PASS: files are byte-for-byte identical.` for every file in the corpus.

## Compilation and Execution

Windows:

```text
g++ -std=c++11 test_output_writer.cpp output_writer.cpp -o test_output_writer
.\test_output_writer.exe

g++ -std=c++11 verify_integrity.cpp output_writer.cpp -o verify-integrity
.\verify-integrity.exe original.bin restored.bin
```

Linux:

```text
g++ -std=c++11 test_output_writer.cpp output_writer.cpp -o test_output_writer
./test_output_writer

g++ -std=c++11 verify_integrity.cpp output_writer.cpp -o verify-integrity
./verify-integrity original.bin restored.bin
```
