#include "ProgramLoader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<uint32_t> ProgramLoader::loadProgram(
    const std::string& filename
) {
    std::vector<uint32_t> program;

    // Open the program file for reading.
    std::ifstream file(filename);

    // If the file could not be opened, stop with a clear error.
    if (!file.is_open()) {
        throw std::runtime_error("Could not open program file: " + filename);
    }

    std::string line;

    // Read the file one line at a time.
    while (std::getline(file, line)) {

        // Ignore empty lines.
        if (line.empty()) {
            continue;
        }

        // Convert the hexadecimal string into a 32-bit integer.
        uint32_t instruction;

        std::stringstream ss(line);
        ss >> std::hex >> instruction;

        // Make sure the conversion was successful.
        if (ss.fail()) {
            throw std::runtime_error(
                "Invalid instruction in program file: " + line
            );
        }

        // Store the decoded machine instruction.
        program.push_back(instruction);
    }

    return program;
}