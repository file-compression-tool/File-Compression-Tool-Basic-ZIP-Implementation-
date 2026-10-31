# Decompressor Module Tests

## 1. Valid File Tests

| Test Case | Input File | Expected Result | Status |
|---|---|---|---|
| Normal text file | sample.huff | Original content restored correctly | PASS |
| Empty file | empty.huff | Creates an empty output file (0 bytes) | PASS |
| Single-character file | single.huff | Reconstructs all six A characters | PASS |

## 2. Integrity Verification

- Compared original and decompressed normal files using `fc.exe /b`.
- Compared original and decompressed single-character files using `fc.exe /b`.
- Both comparisons reported: `FC: no differences encountered`.

## 3. Error Handling Tests
## 3. Error Handling Tests

| Test Case | Expected Result | Status |
|---|---|---|
| Invalid magic signature | Rejects the corrupted HUFF signature | PASS |
| Unsupported file version | Rejects version 2 | PASS |
| Incomplete header | Rejects a file containing only 10 header bytes | PASS |
| Corrupted frequency table | Rejects duplicate symbols in the frequency table | PASS |
| Truncated payload | Rejects a compressed file missing its last byte | PASS |
| Invalid padding bits | Rejects non-zero padding bits in the final byte | PASS |
| Unexpected extra data | Rejects extra bytes after the compressed payload | PASS |
| Existing output file | Refuses to overwrite an existing file | PASS |


