#!/usr/bin/env bash
# End-to-end INTEGRATION tests: drives the real file-compressor program
# (CLI -> reader -> frequency -> Huffman tree -> encoder -> .huff file -> decoder -> output)
# and checks the restored file is byte-for-byte identical to the original.
#
#   bash quality/run_integration_tests.sh
# Optional: FC_BIN=/path/to/file-compressor to test an already built program.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/quality/reports"
REPORT="$OUT/integration_test_report.txt"
mkdir -p "$OUT"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

if [ -z "${FC_BIN:-}" ]; then
  FC_BIN="$WORK/file-compressor"
  ( cd "$ROOT" && g++ -std=c++11 -Wall -Wextra main.cpp cli.cpp file_reader.cpp \
      frequency.cpp huffman_tree.cpp encoder.cpp decoder.cpp -o "$FC_BIN" ) || { echo "BUILD FAILED"; exit 1; }
fi
cd "$WORK" || exit 1

PASS=0; FAIL=0
{ echo "INTEGRATION TEST REPORT - File Compression Tool"
  echo "Program under test: file-compressor (full CLI pipeline)"
  echo "Date: $(date '+%Y-%m-%d %H:%M')"
  echo "======================================================================"; } > "$REPORT"

record() {  # record <PASS|FAIL> <id> <description>
  if [ "$1" = PASS ]; then PASS=$((PASS+1)); else FAIL=$((FAIL+1)); fi
  printf '%-5s %-8s %s\n' "$1" "$2" "$3" | tee -a "$REPORT"
}

roundtrip() {  # roundtrip <id> <description> <file>
  local id="$1" desc="$2" f="$3"
  if "$FC_BIN" compress "$f" -o "$f.huff" --force >/dev/null 2>&1 \
     && "$FC_BIN" decompress "$f.huff" -o "$f.restored" --force >/dev/null 2>&1 \
     && cmp -s "$f" "$f.restored"; then
    record PASS "$id" "$desc"
  else
    record FAIL "$id" "$desc"
  fi
}

echo "--- A. Round-trip tests: compress -> decompress -> compare ---" | tee -a "$REPORT"
: > empty.bin
printf 'A' > onebyte.bin
head -c 1000 /dev/zero | tr '\0' 'x' > single_symbol.bin
printf 'The quick brown fox jumps over the lazy dog.\nHuffman coding assigns shorter codes to frequent bytes.\n' > text.txt
printf 'line1\r\nline2\r\nline3\r\n' > crlf.txt
for i in $(seq 1 200); do echo "repeated line number $i with some repeated words words words"; done > large_text.txt
python3 -c "import sys; sys.stdout.buffer.write(bytes(range(256)))" > all256.bin 2>/dev/null \
  || { for i in $(seq 0 255); do printf "\\$(printf '%03o' $i)"; done > all256.bin; }
head -c 1048576 /dev/urandom > random1mb.bin

roundtrip IT-01 "Empty file (0 bytes)"                      empty.bin
roundtrip IT-02 "Single byte file"                          onebyte.bin
roundtrip IT-03 "Single repeated symbol (1000 x 'x')"       single_symbol.bin
roundtrip IT-04 "Small text file"                           text.txt
roundtrip IT-05 "Windows line endings (CRLF preserved)"     crlf.txt
roundtrip IT-06 "Larger repetitive text (200 lines)"        large_text.txt
roundtrip IT-07 "Binary file with all 256 byte values"      all256.bin
roundtrip IT-08 "1 MB random binary (incompressible)"       random1mb.bin

echo "--- B. Compression effectiveness ---" | tee -a "$REPORT"
orig=$(wc -c < large_text.txt); comp=$(wc -c < large_text.txt.huff)
if [ "$comp" -lt "$orig" ]; then
  record PASS IT-09 "Repetitive text gets smaller ($orig -> $comp bytes)"
else
  record FAIL IT-09 "Repetitive text did not shrink ($orig -> $comp bytes)"
fi

echo "--- C. Error handling and security ---" | tee -a "$REPORT"
"$FC_BIN" compress does_not_exist.txt >/dev/null 2>&1
[ $? -ne 0 ] && record PASS IT-10 "Missing input file is rejected (non-zero exit)" \
             || record FAIL IT-10 "Missing input file was NOT rejected"

printf 'this is not a huff file at all' > fake.huff
"$FC_BIN" decompress fake.huff -o fake.out --force >/dev/null 2>&1
[ $? -ne 0 ] && record PASS IT-11 "File with invalid header is rejected" \
             || record FAIL IT-11 "Invalid header was NOT rejected"

head -c 20 text.txt.huff > truncated.huff
"$FC_BIN" decompress truncated.huff -o trunc.out --force >/dev/null 2>&1
[ $? -ne 0 ] && record PASS IT-12 "Truncated .huff file is rejected" \
             || record FAIL IT-12 "Truncated .huff file was NOT rejected"

# flip a byte near the end (compressed data) of a valid file
cp large_text.txt.huff corrupt.huff
sz=$(wc -c < corrupt.huff)
printf '\377' | dd of=corrupt.huff bs=1 seek=$((sz-2)) conv=notrunc 2>/dev/null
"$FC_BIN" decompress corrupt.huff -o corrupt.out --force >/dev/null 2>&1
rc=$?
if [ $rc -ne 0 ] || ! cmp -s large_text.txt corrupt.out; then
  record PASS IT-13 "Corrupted data is detected or does not silently match original"
else
  record FAIL IT-13 "Corrupted data silently produced the original (unexpected)"
fi

echo "keep me" > existing.txt
"$FC_BIN" compress text.txt -o existing.txt </dev/null >/dev/null 2>&1
if [ $? -ne 0 ] && grep -qax "keep me" existing.txt; then
  record PASS IT-14 "Existing output is NOT overwritten without confirmation"
else
  record FAIL IT-14 "Existing output was overwritten without confirmation"
fi

"$FC_BIN" compress text.txt -o existing.txt --force >/dev/null 2>&1
if [ $? -eq 0 ] && ! grep -qax "keep me" existing.txt; then
  record PASS IT-15 "--force overwrites existing output"
else
  record FAIL IT-15 "--force did not overwrite existing output"
fi

"$FC_BIN" bogus-command >/dev/null 2>&1
[ $? -ne 0 ] && record PASS IT-16 "Unknown command is rejected" || record FAIL IT-16 "Unknown command accepted"

"$FC_BIN" --help >/dev/null 2>&1
[ $? -eq 0 ] && record PASS IT-17 "--help works (exit 0)" || record FAIL IT-17 "--help failed"

"$FC_BIN" compress text.txt -o /nonexistent_dir/out.huff --force >/dev/null 2>&1
[ $? -ne 0 ] && record PASS IT-18 "Unwritable output path is reported as failure" \
             || record FAIL IT-18 "Unwritable output path reported success"

{ echo "======================================================================"
  echo "TOTAL: $((PASS+FAIL)) tests   PASSED: $PASS   FAILED: $FAIL"; } | tee -a "$REPORT"
[ "$FAIL" -eq 0 ]
