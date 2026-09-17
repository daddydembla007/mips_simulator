#ifndef PROGRAM_LOADER_H
#define PROGRAM_LOADER_H

#include <cstdint>
#include <string>
#include <vector>

class ProgramLoader {
public:
    // Reads machine-code instructions from a text file.
    // Each line should contain one 32-bit instruction in hexadecimal.
    static std::vector<uint32_t> loadProgram(const std::string& filename);
};

#endif