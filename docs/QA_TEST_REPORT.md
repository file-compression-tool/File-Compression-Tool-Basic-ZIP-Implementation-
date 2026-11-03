# QA Test Execution Report

## Run information

- Date: 2026-11-03
- Environment: Windows, GCC 16.2.0 (MSYS2 UCRT64)
- Build flags: `-std=c++11 -Wall -Wextra`
- Scope: Unit, integration, CLI, round-trip, malformed-input, and overwrite checks.

## Test suite results

All nine test programs built and exited successfully:

| Test program | Result | Coverage |
|---|---|---|
| `test_cli` | PASS | CLI parsing and help, compression/decompression, empty and binary files, invalid compressed files, overwrite handling |
| `test_frequency` | PASS | Empty, known-frequency, single-symbol, binary, and boundary inputs |
| `test_huffman_tree` | PASS | Empty/single-symbol trees, equal frequencies, deterministic prefix-free codes, malformed trees |
| `test_encoder` | PASS | Header/bitstream writing and invalid-code/output handling |
| `test_decoder` | PASS | Valid round trips and malformed header, metadata, and payload rejection |
| `test_output_writer` | PASS | Binary byte preservation, large writes, invalid paths, overwrite protection, file comparison |
| `test_decompressor` | PASS | Existing output is rejected and remains unchanged |
| `test_reader` | PASS | Reads a fixture and reports a missing input |
| `test_integration` | PASS | File read through frequency analysis and Huffman encoding |

`test_reader` and `test_integration` were run with temporary sample input. The
decompressor test was updated to create its own compressed fixture, verify that
an existing output remains unchanged, and return a failing exit code if its
assertions fail. This removes its prior dependency on external fixture files
and its success-shaped exit status.

## End-to-end round trips

The compiled command-line executable compressed and decompressed each input.
The restored output matched the original bytes in every case.

| Input | Original size | Compressed size | Result | Elapsed time |
|---|---:|---:|---|---:|
| Empty | 0 bytes | 23 bytes | PASS | 72 ms |
| Repeated byte | 65,536 bytes | 8,224 bytes | PASS | 100 ms |
| Text | 740,000 bytes | 385,203 bytes | PASS | 699 ms |
| Binary pattern | 1,048,576 bytes | 1,050,903 bytes | PASS | 1,459 ms |

Timings are a local smoke measurement, not a performance benchmark or SLA.

## Malformed-input checks

The decoder rejected 100 deterministic random inputs of 0-128 bytes. None
returned success or created an output file. The decoder and CLI suites also
rejected malformed/truncated headers and corrupted payloads.

## Requirement traceability and remaining validation

| SRS requirements | Result | Evidence / limitation |
|---|---|---|
| FCT-F-001 to FCT-F-016 | Tested | CLI, module, and integration suites cover the functional paths, error cases, and overwrite behavior listed above. |
| FCT-NF-001 | PASS | Byte-for-byte round trips passed for all four end-to-end inputs. |
| FCT-NF-002 | PARTIAL | Smoke timings recorded; no measurable acceptance threshold or controlled baseline is defined. |
| FCT-NF-003 | NOT MEASURED | Peak memory and resource limits were not measured. |
| FCT-NF-004 | PARTIAL | Automated help/usage checks passed; no independent user usability session was run. |
| FCT-NF-005 | PARTIAL | Build/tests ran on Windows with GCC; Linux portability was not tested in this run. |
| FCT-NF-006 | PARTIAL | Corrupt-input and invalid-path failures were tested; injected mid-read/write failures were not. |
| FCT-SR-001, FCT-SR-005 | PASS for tested cases | Malformed/truncated headers and payloads, plus 100 random short inputs, were rejected. This is not exhaustive fuzzing. |
| FCT-SR-002 | PARTIAL | Missing-input and invalid-output-path failures were tested; simulated I/O failures were not. |
| FCT-SR-003 | PASS | Existing output was preserved unless overwrite was explicitly confirmed or forced. |
| FCT-SR-004 | PASS for tested cases | Empty and non-existent output paths were rejected. Platform-specific permission-denied cases were not tested. |

## Overall outcome

All nine test programs and all four end-to-end data round trips passed in this
run. The test plan is not fully complete: memory measurements, Linux
portability, a user usability session, controlled performance targets, and
injected I/O failures remain outstanding.

The GitHub Actions workflow now builds and runs the module and integration test
programs in addition to the CLI tests. The updated workflow still needs a
GitHub Actions run after these changes are pushed.
