#!/usr/bin/env bash
# Builds every test suite with coverage instrumentation, runs them,
# and produces coverage reports in quality/reports/.
# Run from anywhere:  bash quality/run_coverage.sh
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/quality/reports"
BUILD="$(mktemp -d)"
mkdir -p "$OUT"
cd "$ROOT"

CXX="g++ -std=c++11 -O0 -g --coverage -Wall -Wextra"

# name | sources  (same source lists as .github/workflows/ci.yml)
SUITES=(
  "test_cli|test_cli.cpp cli.cpp file_reader.cpp frequency.cpp huffman_tree.cpp encoder.cpp decoder.cpp"
  "test_frequency|test_frequency.cpp frequency.cpp file_reader.cpp huffman_tree.cpp"
  "test_huffman_tree|test_huffman_tree.cpp huffman_tree.cpp"
  "test_encoder|test_encoder.cpp encoder.cpp"
  "test_decoder|test_decoder.cpp decoder.cpp encoder.cpp huffman_tree.cpp"
  "test_output_writer|test_output_writer.cpp output_writer.cpp"
  "test_decompressor|test_decompressor.cpp decompressor.cpp encoder.cpp huffman_tree.cpp"
  "test_reader|test_reader.cpp file_reader.cpp"
  "test_integration|test_integration.cpp file_reader.cpp frequency.cpp huffman_tree.cpp encoder.cpp"
)

: > "$OUT/test_results.txt"
for entry in "${SUITES[@]}"; do
  name="${entry%%|*}"; srcs="${entry#*|}"
  # each suite gets its own object dir so .gcda files do not collide
  mkdir -p "$BUILD/$name"
  # shellcheck disable=SC2086
  if ! $CXX $srcs -o "$BUILD/$name/$name" 2>"$BUILD/$name/build.log"; then
    echo "BUILD FAILED: $name" | tee -a "$OUT/test_results.txt"
    cat "$BUILD/$name/build.log" >> "$OUT/test_results.txt"
    continue
  fi
  # run inside a scratch dir so tests do not litter the repo
  work="$BUILD/$name/run"; mkdir -p "$work"
  printf 'CI integration fixture for file-reader and encoder tests.\n' > "$work/sample.txt"
  ( cd "$work" && "$BUILD/$name/$name" >"$BUILD/$name/out.log" 2>&1 )
  rc=$?
  [ $rc -eq 0 ] && status=PASS || status="FAIL(exit $rc)"
  echo "$status  $name" | tee -a "$OUT/test_results.txt"
done

# Merge coverage from all suites. Only project sources, not tests or system headers.
gcovr --root "$ROOT" "$BUILD" \
  --filter "$ROOT/[a-z_]*\.cpp" --exclude "$ROOT/test_.*" \
  --exclude "$ROOT/generate_sample.cpp" --exclude "$ROOT/verify_integrity.cpp" \
  --gcov-ignore-parse-errors=negative_hits.warn_once_per_file \
  --txt "$OUT/coverage_summary.txt" --print-summary \
  --html-details "$OUT/coverage.html" \
  --xml "$OUT/coverage.xml" \
  --json-summary "$OUT/coverage_summary.json" 2>&1 | tail -5

# Branch-coverage table too
gcovr --root "$ROOT" "$BUILD" \
  --filter "$ROOT/[a-z_]*\.cpp" --exclude "$ROOT/test_.*" \
  --exclude "$ROOT/generate_sample.cpp" --exclude "$ROOT/verify_integrity.cpp" \
  --gcov-ignore-parse-errors=negative_hits.warn_once_per_file \
  --txt-metric branch --txt "$OUT/branch_coverage.txt" >/dev/null 2>&1

rm -rf "$BUILD"
echo; echo "Reports written to $OUT"
