#include "decompressor.hpp"

#include <iostream>

int main()
{
    std::cout << "===== Decompressor Module Tests =====\n";

    bool result = decompressFile(
      "sample.huff",
      "existing_output.txt"
    );

   if (!result)
{
    std::cout << "PASS: Existing output file was protected.\n";
}
else
{
    std::cout << "FAIL: Existing output file was overwritten.\n";
}
    return 0;
}