Software Test Plan (STP)
- Project:File compression (zip file basic implementation)
- Version: 1.0 
- Authors:Team5
- Date: 16-09-2026
- Status: Draft

  
1. Introduction
   
- Purpose: This document defines the test plan for the File Compression Tool v1.0. It outlines the objectives, scope, strategy, resources, schedule, and responsibilities for testing the compression and decompression functionality built on Huffman coding.

- Scope:Test scope includes the entire process of CLI tool usage: file reading, frequency analysis, Huffman tree creation, codes creation, bits string creation, metadata/header storing, compressed file creation, file decompression, header integrity, and exact reproduction of original file. GUI testing, Zip file format compatibility, and behavior on servers/networks are not part of test scope since it is a CLI utility without these features.

- References: File Compression Tool SRS v1.0, design specifications (DS-IO, DS-HUFF, DS-ENC, DS-FMT, DS-DEC, DS-ERR, DS-SEC series), RTM v1.0.

- Definitions: SRS (Software requirements specification), RTM (requirements traceability matrix), CLI (Command Line Interface), NFR (Non-functional requirement), TC (Test Case).

2. Test Items 
- File I/O Module
- Frequency Analysis Module
- Huffman Tree / Code Generation Module
- Encoder Module (bitstream + header writing)
- File Format Module (metadata/header structure)
- Decoder Module (header validation, tree reconstruction, bitstream decoding)
- Error Handling Module
- CLI / UI Module (prompts, messages, statistics display)


3. Features to be Tested 
Features mapped to SRS requirement IDs: 
- FCT-F-001 / FCT-F-002: Accept and read input file (binary, unaltered) — TC-COMP-01, TC-COMP-02
- FCT-F-003: Frequency calculation — TC-HUFF-01
- FCT-F-004 / FCT-F-005: Huffman tree construction and prefix-free code generation — TC-HUFF-02, TC-HUFF-03
- FCT-F-006 / FCT-F-007 / FCT-F-008: Bitstream encoding, header write, output file creation — TC-COMP-03, TC-FMT-01, TC-COMP-04
- FCT-F-009: Compression statistics display — TC-UI-01
- FCT-F-010 / FCT-F-011: Accept and validate compressed file — TC-DECOMP-01, TC-DECOMP-02
- FCT-F-012 / FCT-F-013: Reconstruct Huffman structure and decode bitstream — TC-HUFF-04, TC-DECOMP-03
- FCT-F-014: Write decompressed output file — TC-DECOMP-04
- FCT-F-015 / FCT-F-016: Error handling and overwrite protection — TC-ERR-01, TC-ERR-02
- FCT-NF-001: Byte-for-byte data integrity — TC-NF-01
- FCT-NF-002 / FCT-NF-003: Processing performance and bounded memory use — TC-PERF-01, TC-PERF-02
- FCT-NF-004: CLI usability — TC-UX-01
- FCT-NF-005: Portability (Windows/Linux build) — TC-PORT-01
- FCT-NF-006: No silent data corruption — TC-REL-01
- FCT-SR-001 through FCT-SR-005: Metadata validation, access-failure reporting, overwrite prevention, output-path validation, corrupted-header rejection — TC-SEC-01 through TC-SEC-05

4. Features Not to be Tested 
- Any graphical user interface (the tool is CLI-only)
- External server, network, or database behavior (none exists in this system)
- Compiler/toolchain internals (GCC/G++/MSVC correctness is assumed, not tested)

5. Test Approach / Strategy 
Levels: 
- Module-level unit tests: frequency counter, tree builder, code generator, bit-level reader/writer, independently of other components
- Integration testing: complete pipeline of encode/decode + write/read operations within the tool
- System level tests: end-to-end command-line compression/decompression on files
- Acceptance tests: validation against SRS §7 criteria of successful completion, approved by the course evaluator

Types: 
- Functional Testing (core functionalities of compression/decompression)
- Regression Testing (rerunning complete set of tests after every fix)
- Performance Testing (time and memory utilization with respect to file size, FCT-NF-002/003)
- Reliability/Integrity Testing (byte-to-byte comparison of original file and decompressed file)
- Usability Testing (prompt/message of command line interface, FCT-NF-004)
- Security Testing (handling corrupted/input handling)

Entry Criteria: Stable build is compiled free of errors; test files (empty, single symbol, small text, binary, large file) are available; test environment is set up.

Exit Criteria: All high priority test cases planned are executed; no critical bugs found (any data integrity or corruption bug is a critical bug); all acceptance criteria mentioned in SRS Chapter 7 are met; RTM completed with Pass/Fail status of all 27 requirements.

5.1 Security Validation 
- Verify rejection of malformed/corrupt compressed-file header 
- Ensure false "success" is not reported when reads/writes fail 
- Ensure overwrite protection messages work as expected and cannot be accidentally circumvented 
- Test with invalid/inaccessible output directories (nonexistent/unaccessible directory
- Simple fuzzing of the compressed-file header using random/defective byte sequences to ensure that there is no crash or undefined behavior


6. Test Environment 
Hardware: Standard desktop/laptop, minimum 4 GB RAM, with local storage for test files.
Software: File Compression Tool v1.0 build; C/C++ compiler toolchain (GCC/G++ on Linux, MSVC on Windows); target OS: Windows and Linux, per FCT-NF-005 portability requirement.


Tools: Git/GitHub Issues for defect tracking 
Shell scripting  command for basic performance benchmarking (FCT-NF-002) 
Test Data: A sample corpus of file – blank file, file with unique symbol only, small text file, large text file, small binary file, large binary file, and corrupt/truncated files to test the compression/decompression.

7. Test Schedule 
Milestones: 
- Test case design: 05-Sep-2026
- Environment setup: 07-Sep-2026
- Test execution start: 08-Sep-2026
- Test execution end: 20-Sep-2026
- UAT: 22-Sep-2026 to 25-Sep-2026

8. Test Deliverables 
- Test Plan (the above document) - Defines objectives, scope, strategy, schedule, and responsibilities. 
- Test Case (manual/automated test cases correlated with RTM) 
  - Functional - Manual test cases for FCT-F-001 to FCT-F-016 covering compression, decompression, and error handling. 
  - NFR & Security - Test cases for FCT-NF-001 to FCT-NF-006 and FCT-SR-001 to FCT-SR-005. 
- Test Data (file repository) - Prepared test input files: empty, single-symbol, small/medium text, binary, large file. 
- Test Logs - CLI output screenshots or text logs capturing pass/fail results for each test case. 
- Defect Log - GitHub Issues raised for each failed test, with steps to reproduce and severity. 
- Test Summary Report - Final report summarising % passed/failed, open defects, requirement coverage, and sign-off recommendation. 
- Latest execution results: [QA Test Execution Report](./docs/QA_TEST_REPORT.md) (2026-11-03), including passed suites and outstanding validation.
- RTM (updated) - RTM with Status column filled (P/N/A) for every FCT-F, FCT-NF, and FCT-SR requirement.



9. Roles and Responsibilities 


| Role | Name | Responsibility |
| -------- | -------- | -------- |
| QA Lead  | Karthik | Prepare plan, coordinate execution |
| Test Engineer  | Pavithra  | Design & execute test cases for compression (FCT-F-001–009) and NFR suites , log defects|
| Test Engineer  | Kavan  | Design and execute test cases for decompression (FCT-F-010–014), error handling, and security suites; log defects.   |
| Developer | Vaishnavi  | Implement fixes for logged defects; support defect triage; assist with test environment setup.  |

10. Risks and Mitigation 

| Risk  | Mitigation| 
| -------- | -------- | 
|Delay in stable build delivery| Request an early smoke build; define a minimum viable binary that can at least compress one file.|
|Test environment downtime | Set up an environment on at least two team members' machines, use an online C++ compiler as fallback. |
|Byte-for-byte integrity failure (core defect)| Prioritise TC-NF-01 integrity suite early; block decompression testing until compression output is verified stable. |
|Limited time for performance and security testing| Treat NFR and security test cases as a separate phase; core functional tests take priority for submission.|

11. Assumptions & Dependencies 
- The SRS v1.0 is baseline and will not change significantly after the test plan is approved.
-  Each team member has access to a computer with a C/C++ compiler and CLI environment.
-   Test data files (empty, single-symbol, small/medium/large text and binary) will be prepared by the team before test execution begins.
-   GitHub Issues will be used for defect tracking; all team members have access to the repository.
-    The header/metadata format for the compressed file will be agreed and documented before integration testing begins.
-   Diff or fc (file compare) utilities are available on the test machine for byte-level comparison.

12. Suspension & Resumption Criteria
    
Suspend testing if: 
- The build does not compile on the agreed platform (blocks all testing).
-    A critical defect blocks more than 30% of planned test cases from executing.
-    The test environment (compiler, file system) is unavailable for more than 4 hours.
-    A byte-for-byte integrity failure (TC-NF-01) is discovered — do not proceed with further compression testing until the root cause is fixed.

Resume testing if:

-   A fixed build is delivered and passes an initial smoke test (compress + decompress one file).
-    Blocking critical defects are resolved and verified and fixed by the developer.
-    The test environment is restored and verified.


13. Test Case Management & Traceability :
The RTM ensures every SRS requirement maps to at least one test case. Examples:
- FCT-F-004 (Build Huffman tree) → TC-HUFF-02
- FCT-F-013 (Decode bitstream) → TC-DECOMP-03
- FCT-NF-001 (Data integrity) → TC-NF-01
- FCT-SR-005 (Reject invalid compressed headers) → TC-SEC-05
Full traceability for all 27 requirements is maintained in the RTM document accompanying this test plan.

14. Test Metrics & Reporting
    
Metrics collected:
- % of test cases executed
- % passed / failed
- Defect count by severity (critical / major / minor)
- Defect aging (time open)
- Requirement coverage (% of RTM rows with a passing test)
- Byte-for-byte integrity pass rate across the file corpus


