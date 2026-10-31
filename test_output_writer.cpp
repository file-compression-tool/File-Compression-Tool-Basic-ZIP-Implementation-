#include "output_writer.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace std;

static vector<uint8_t> readAll(const string& path)
{
    ifstream in(path.c_str(), ios::binary);
    return vector<uint8_t>((istreambuf_iterator<char>(in)),
                           istreambuf_iterator<char>());
}

static void writeRaw(const string& path, const vector<uint8_t>& data)
{
    ofstream out(path.c_str(), ios::binary);
    if (!data.empty())
        out.write(reinterpret_cast<const char*>(&data[0]), data.size());
}

// OUT-01: bytes on disk are exactly the bytes in memory, including 0x0A / 0x0D
// (text-mode output would change these on Windows) and 0x00 / 0xFF
static void testExactBytes()
{
    vector<uint8_t> data;
    data.push_back('a');
    data.push_back(0x0A);
    data.push_back(0x0D);
    data.push_back(0x0A);
    data.push_back(0x00);
    data.push_back(0xFF);
    data.push_back(0x1A);

    remove("out_exact.bin");
    assert(writeOutputFile(data, "out_exact.bin"));
    assert(readAll("out_exact.bin") == data);
    remove("out_exact.bin");
    cout << "OUT-01 passed\n";
}

// OUT-02: every one of the 256 byte values survives
static void testAll256()
{
    vector<uint8_t> data;
    for (int i = 0; i < 256; ++i)
        data.push_back(static_cast<uint8_t>(i));

    remove("out_256.bin");
    assert(writeOutputFile(data, "out_256.bin"));
    assert(readAll("out_256.bin") == data);
    remove("out_256.bin");
    cout << "OUT-02 passed\n";
}

// OUT-03: empty decoded data -> empty file is created
static void testEmptyData()
{
    remove("out_empty.bin");
    assert(writeOutputFile(vector<uint8_t>(), "out_empty.bin"));
    assert(readAll("out_empty.bin").empty());
    remove("out_empty.bin");
    cout << "OUT-03 passed\n";
}

// OUT-04: large output (1 MB)
static void testLarge()
{
    vector<uint8_t> data(1024 * 1024);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<uint8_t>((i * 31 + 7) % 256);

    remove("out_large.bin");
    assert(writeOutputFile(data, "out_large.bin"));
    assert(readAll("out_large.bin") == data);
    remove("out_large.bin");
    cout << "OUT-04 passed\n";
}

// OUT-05: existing file + no overwrite -> refused, file untouched
static void testNoOverwrite()
{
    vector<uint8_t> original(3, 'A');
    writeRaw("out_exist.bin", original);

    assert(!writeOutputFile(vector<uint8_t>(5, 'Z'), "out_exist.bin", false));
    assert(readAll("out_exist.bin") == original);
    remove("out_exist.bin");
    cout << "OUT-05 passed\n";
}

// OUT-06: existing file + overwrite -> replaced completely
static void testOverwrite()
{
    writeRaw("out_over.bin", vector<uint8_t>(100, 'A'));

    vector<uint8_t> replacement(5, 'Z');
    assert(writeOutputFile(replacement, "out_over.bin", true));
    assert(readAll("out_over.bin") == replacement); // no leftover old bytes
    remove("out_over.bin");
    cout << "OUT-06 passed\n";
}

// OUT-07: folder does not exist -> controlled failure, nothing created
static void testBadFolder()
{
    assert(!writeOutputFile(vector<uint8_t>(3, 'x'),
                            "no_such_folder_xyz/out.bin"));
    ifstream check("no_such_folder_xyz/out.bin", ios::binary);
    assert(!check.is_open());
    cout << "OUT-07 passed\n";
}

// OUT-08: empty path -> controlled failure
static void testEmptyPath()
{
    assert(!writeOutputFile(vector<uint8_t>(3, 'x'), ""));
    cout << "OUT-08 passed\n";
}

// INT-01: identical files
static void testIdentical()
{
    vector<uint8_t> data;
    for (int i = 0; i < 70000; ++i) // spans more than one 64 KB chunk
        data.push_back(static_cast<uint8_t>(i % 251));
    writeRaw("int_a.bin", data);
    writeRaw("int_b.bin", data);
    assert(filesAreIdentical("int_a.bin", "int_b.bin"));
    remove("int_a.bin");
    remove("int_b.bin");
    cout << "INT-01 passed\n";
}

// INT-02: one byte differs deep in the file (second chunk)
static void testOneByteDifferent()
{
    vector<uint8_t> a(70000, 'k');
    vector<uint8_t> b = a;
    b[66000] = 'X';
    writeRaw("int_a.bin", a);
    writeRaw("int_b.bin", b);
    assert(!filesAreIdentical("int_a.bin", "int_b.bin"));
    remove("int_a.bin");
    remove("int_b.bin");
    cout << "INT-02 passed\n";
}

// INT-03: different lengths (e.g. a missing trailing byte)
static void testDifferentLength()
{
    writeRaw("int_a.bin", vector<uint8_t>(10, 'a'));
    writeRaw("int_b.bin", vector<uint8_t>(9, 'a'));
    assert(!filesAreIdentical("int_a.bin", "int_b.bin"));
    remove("int_a.bin");
    remove("int_b.bin");
    cout << "INT-03 passed\n";
}

// INT-04: CRLF vs LF is detected as a difference
static void testLineEndingDifference()
{
    vector<uint8_t> lf;
    lf.push_back('h'); lf.push_back('i'); lf.push_back(0x0A);
    vector<uint8_t> crlf;
    crlf.push_back('h'); crlf.push_back('i');
    crlf.push_back(0x0D); crlf.push_back(0x0A);
    writeRaw("int_a.bin", lf);
    writeRaw("int_b.bin", crlf);
    assert(!filesAreIdentical("int_a.bin", "int_b.bin"));
    remove("int_a.bin");
    remove("int_b.bin");
    cout << "INT-04 passed\n";
}

// INT-05: two empty files are identical
static void testBothEmpty()
{
    writeRaw("int_a.bin", vector<uint8_t>());
    writeRaw("int_b.bin", vector<uint8_t>());
    assert(filesAreIdentical("int_a.bin", "int_b.bin"));
    remove("int_a.bin");
    remove("int_b.bin");
    cout << "INT-05 passed\n";
}

// INT-06: a missing file is a failure, not a crash
static void testMissingFile()
{
    writeRaw("int_a.bin", vector<uint8_t>(3, 'a'));
    assert(!filesAreIdentical("int_a.bin", "int_definitely_missing.bin"));
    assert(!filesAreIdentical("int_definitely_missing.bin", "int_a.bin"));
    remove("int_a.bin");
    cout << "INT-06 passed\n";
}

int main()
{
    testExactBytes();
    testAll256();
    testEmptyData();
    testLarge();
    testNoOverwrite();
    testOverwrite();
    testBadFolder();
    testEmptyPath();
    testIdentical();
    testOneByteDifferent();
    testDifferentLength();
    testLineEndingDifference();
    testBothEmpty();
    testMissingFile();

    cout << "\nAll output writer and integrity tests passed.\n";
    return 0;
}
