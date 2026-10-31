// Command-line integrity check for TC-NF-01:
//   verify-integrity <original-file> <restored-file>
// Exit code 0 = byte-for-byte identical, 1 = different/unreadable, 2 = usage.
#include "output_writer.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: verify-integrity <original-file> <restored-file>"
                  << std::endl;
        return 2;
    }

    if (filesAreIdentical(argv[1], argv[2]))
    {
        std::cout << "PASS: files are byte-for-byte identical." << std::endl;
        return 0;
    }

    std::cout << "FAIL: files are not identical." << std::endl;
    return 1;
}
