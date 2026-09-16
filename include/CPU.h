#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <vector>

#include "Decoder.h"

class CPU {
public:
    // Program Counter: stores the address of the next instruction
    uint32_t PC;

    // MIPS has 32 general-purpose registers, each 32 bits wide
    uint32_t registers[32];

    // Instruction memory: each instruction is 32 bits
    std::vector<uint32_t> instructionMemory;

    // Data memory is byte-addressable
    std::vector<uint8_t> dataMemory;

    CPU();

    // Reset the processor to its initial state
    void reset();

    // Read a value from one of the 32 registers
    uint32_t readRegister(uint8_t index) const;

    // Write a value to a register
    void writeRegister(uint8_t index, uint32_t value);

    // Read a 32-bit word from memory
    uint32_t readMemoryWord(uint32_t address) const;

    // Write a 32-bit word to memory
    void writeMemoryWord(uint32_t address, uint32_t value);

    // Execute one decoded instruction
    void execute(const DecodedInstruction& instruction);
};

#endif