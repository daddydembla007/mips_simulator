#ifndef DECODER_H
#define DECODER_H

#include "Instruction.h"

// Represents the actual operation that the CPU needs to perform
enum class Operation {
    NOP,

    // R-type arithmetic and logical operations
    ADD,
    SUB,
    AND,
    OR,
    SLT,

    // I-type operations
    ADDI,
    LW,
    SW,

    // Branch and jump operations
    BEQ,
    BNE,
    J,

    // Used when the instruction is not supported/recognized
    INVALID
};

// Contains the useful information obtained after decoding
// a raw MIPS instruction.
struct DecodedInstruction {

    Operation operation;

    // Register numbers
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;

    // 16-bit immediate value for I-type instructions
    int16_t immediate;

    // 26-bit address field for J-type instructions
    uint32_t address;

    // True when the instruction uses an immediate operand
    bool usesImmediate;
};

// Converts the fields extracted from Instruction into
// a meaningful CPU operation such as ADD, LW, BEQ, etc.
DecodedInstruction decodeInstruction(
    const Instruction& instruction
);

#endif