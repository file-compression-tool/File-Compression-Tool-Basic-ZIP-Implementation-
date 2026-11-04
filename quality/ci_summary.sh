#!/usr/bin/env bash
# Prints a Markdown summary of all quality results.
# In GitHub Actions it is appended to the run page:  bash quality/ci_summary.sh >> "$GITHUB_STEP_SUMMARY"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
R="$ROOT/quality/reports"

echo "## Quality report"
echo
echo "### Test suites"
echo '```'
cat "$R/test_results.txt" 2>/dev/null
echo '```'
echo
echo "### Integration tests (end-to-end)"
echo '```'
grep -E '^(PASS|FAIL|TOTAL)' "$R/integration_test_report.txt" 2>/dev/null
echo '```'
echo
echo "### Code coverage"
python3 - "$R/coverage_summary.json" <<'PY'
import json, sys
try:
    d = json.load(open(sys.argv[1]))
except Exception:
    print("_coverage summary not available_"); sys.exit(0)
print(f"**Lines {d['line_percent']:.1f}%** ({d['line_covered']}/{d['line_total']}) | "
      f"**Functions {d['function_percent']:.1f}%** ({d['function_covered']}/{d['function_total']}) | "
      f"**Branches {d['branch_percent']:.1f}%** ({d['branch_covered']}/{d['branch_total']})\n")
print("| File | Lines | Branches | Functions |\n|---|---|---|---|")
for f in sorted(d['files'], key=lambda x: x['line_percent']):
    print(f"| {f['filename']} | {f['line_percent']:.1f}% | {f.get('branch_percent',0):.1f}% | {f.get('function_percent',0):.0f}% |")
PY
echo
echo "### Static analysis"
echo "| Tool | Findings |"
echo "|---|---|"
echo "| cppcheck | $(grep -c ': \[\(warning\|style\|performance\|portability\|error\)\]' "$R/static_cppcheck.txt" 2>/dev/null) |"
echo "| clang-tidy | $(grep -cE 'warning:|error:' "$R/static_clang_tidy.txt" 2>/dev/null) |"
echo "| g++ strict warnings | $(grep -c 'warning:' "$R/static_gcc_warnings.txt" 2>/dev/null) |"
echo
echo "_Full HTML coverage report: download the **quality-reports** artifact from this run._"
