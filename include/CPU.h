#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <vector>

class CPU {
public:
    // Program Counter
    uint32_t PC;

    // 32 general-purpose registers(just like MIPS architecture)
    uint32_t registers[32];

    // Instruction memory
    std::vector<uint32_t> instructionMemory;

    // Data memory
    std::vector<uint8_t> dataMemory;

    CPU();

    void reset();
};

#endif
