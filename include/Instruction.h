#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>

struct Instruction {
    uint32_t raw; // stores entire 32 bit instructions

    uint8_t opcode; 
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    uint8_t shamt;
    uint8_t funct;

    int16_t immediate;
    uint32_t address;

    Instruction(uint32_t instruction);

    void decode();
};

#endif