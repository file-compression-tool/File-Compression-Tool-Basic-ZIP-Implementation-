#!/usr/bin/env bash
# Static analysis = checking the code WITHOUT running it.
# Runs 3 tools and saves results in quality/reports/.
#   bash quality/run_static_analysis.sh
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/quality/reports"
mkdir -p "$OUT"
cd "$ROOT"

# Application code and test code (generate_sample/verify_integrity are helper tools)
SRC="main.cpp cli.cpp file_reader.cpp frequency.cpp huffman_tree.cpp encoder.cpp decoder.cpp decompressor.cpp output_writer.cpp"
TESTS="test_cli.cpp test_frequency.cpp test_huffman_tree.cpp test_encoder.cpp test_decoder.cpp test_output_writer.cpp test_decompressor.cpp test_reader.cpp test_integration.cpp"

echo "[1/3] cppcheck (bugs, memory/resource problems, style)"
# shellcheck disable=SC2086
cppcheck --enable=all --std=c++11 --inconclusive --suppress=missingIncludeSystem \
  --template='{file}:{line}: [{severity}] {id}: {message}' \
  $SRC $TESTS 2> "$OUT/static_cppcheck.txt"
echo "      $(grep -c ': \[' "$OUT/static_cppcheck.txt") findings"

echo "[2/3] clang-tidy (bug-prone patterns, modern C++, performance, readability)"
: > "$OUT/static_clang_tidy.txt"
for f in $SRC; do
  clang-tidy "$f" \
    -checks='-*,bugprone-*,performance-*,modernize-*,readability-*,-readability-magic-numbers,-readability-braces-around-statements,-modernize-use-trailing-return-type' \
    -quiet -- -std=c++11 >> "$OUT/static_clang_tidy.txt" 2>/dev/null
done
echo "      $(grep -cE 'warning:|error:' "$OUT/static_clang_tidy.txt") findings"

echo "[3/3] g++ strict warnings (compiler as a static analyzer)"
# shellcheck disable=SC2086
g++ -std=c++11 -fsyntax-only -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
  -Wold-style-cast -Wuseless-cast -Wnull-dereference \
  $SRC 2> "$OUT/static_gcc_warnings.txt"
echo "      $(grep -c 'warning:' "$OUT/static_gcc_warnings.txt") findings"

echo; echo "Reports written to $OUT"

# Only genuine cppcheck ERRORS (real bugs) fail the pipeline.
# Style / readability findings are reported but do not block.
if grep -q '\[error\]' "$OUT/static_cppcheck.txt"; then
  echo "cppcheck found real errors:"; grep '\[error\]' "$OUT/static_cppcheck.txt"
  exit 1
fi
exit 0
