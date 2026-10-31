#ifndef OUTPUT_WRITER_HPP
#define OUTPUT_WRITER_HPP

#include <cstdint>
#include <string>
#include <vector>

// Writes 'data' to 'outputPath' in BINARY mode so the bytes on disk are exactly
// the bytes in memory (no newline translation on Windows).
//
// - If the file already exists and overwriteExisting is false, nothing is
//   touched and false is returned (FCT-F-016 / FCT-SR-003).
// - An empty path or an unwritable / non-existent folder is reported and
//   returns false (FCT-SR-004).
// - After writing, the size on disk is checked against data.size(); a failed
//   or short write is reported, the partial file is removed and false is
//   returned (no false "success", FCT-SR-002).
bool writeOutputFile(const std::vector<std::uint8_t>& data,
                     const std::string& outputPath,
                     bool overwriteExisting = false);

// Integrity check (FCT-NF-001 / TC-NF-01): returns true only if both files can
// be read and contain exactly the same bytes. On mismatch, the reason (size
// difference or first differing byte offset) is reported on stderr.
bool filesAreIdentical(const std::string& pathA, const std::string& pathB);

#endif
