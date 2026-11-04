#!/usr/bin/env bash
# Builds every test suite with coverage instrumentation, runs them,
# and produces coverage reports in quality/reports/.
# Run from anywhere:  bash quality/run_coverage.sh
#
# Exit code (used by the CI pipeline):
#   0 = all tests passed and coverage >= COVERAGE_MIN_LINE
#   1 = a test failed / did not build, or line coverage is below the minimum
# COVERAGE_MIN_LINE defaults to 0 (no gate) when run by hand; CI sets it.
set -u
MIN_LINE="${COVERAGE_MIN_LINE:-0}"
FAILED=0
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
    FAILED=1
    echo "BUILD FAILED: $name" | tee -a "$OUT/test_results.txt"
    cat "$BUILD/$name/build.log" >> "$OUT/test_results.txt"
    continue
  fi
  # run inside a scratch dir so tests do not litter the repo
  work="$BUILD/$name/run"; mkdir -p "$work"
  printf 'CI integration fixture for file-reader and encoder tests.\n' > "$work/sample.txt"
  ( cd "$work" && "$BUILD/$name/$name" >"$BUILD/$name/out.log" 2>&1 )
  rc=$?
  if [ $rc -eq 0 ]; then status=PASS; else status="FAIL(exit $rc)"; FAILED=1; fi
  echo "$status  $name" | tee -a "$OUT/test_results.txt"
done

# End-to-end integration tests on the real program (adds main.cpp + full CLI flow to coverage)
mkdir -p "$BUILD/integration"
if $CXX main.cpp cli.cpp file_reader.cpp frequency.cpp huffman_tree.cpp encoder.cpp decoder.cpp \
     -o "$BUILD/integration/file-compressor" 2>"$BUILD/integration/build.log"; then
  FC_BIN="$BUILD/integration/file-compressor" bash "$ROOT/quality/run_integration_tests.sh" >/dev/null 2>&1 \
    && echo "PASS  integration (end-to-end CLI, see integration_test_report.txt)" | tee -a "$OUT/test_results.txt" \
    || { FAILED=1; echo "FAIL  integration (end-to-end CLI)" | tee -a "$OUT/test_results.txt"; }
fi

# Merge coverage from all suites. Only project sources, not tests or system headers.
gcovr --root . "$BUILD" \
  --filter "[a-z_]*\.cpp" --exclude "test_.*" \
  --exclude "generate_sample.cpp" --exclude "verify_integrity.cpp" \
  --gcov-ignore-parse-errors=negative_hits.warn_once_per_file \
  --txt "$OUT/coverage_summary.txt" --print-summary \
  --html-details "$OUT/coverage.html" \
  --xml "$OUT/coverage.xml" \
  --json-summary "$OUT/coverage_summary.json" 2>&1 | tail -5

# Branch-coverage table too
gcovr --root . "$BUILD" \
  --filter "[a-z_]*\.cpp" --exclude "test_.*" \
  --exclude "generate_sample.cpp" --exclude "verify_integrity.cpp" \
  --gcov-ignore-parse-errors=negative_hits.warn_once_per_file \
  --txt-metric branch --txt "$OUT/branch_coverage.txt" >/dev/null 2>&1

rm -rf "$BUILD"
echo; echo "Reports written to $OUT"

# Coverage gate: fail if line coverage is below the minimum
LINE_PCT=$(python3 -c "import json;print(json.load(open('$OUT/coverage_summary.json'))['line_percent'])" 2>/dev/null || echo 0)
echo "Line coverage: ${LINE_PCT}%  (required minimum: ${MIN_LINE}%)"
if python3 -c "import sys;sys.exit(0 if float('$LINE_PCT') >= float('$MIN_LINE') else 1)"; then
  :
else
  echo "COVERAGE GATE FAILED: ${LINE_PCT}% is below ${MIN_LINE}%"
  FAILED=1
fi
[ "$FAILED" -eq 0 ]
