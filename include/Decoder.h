#ifndef DECODER_H
#define DECODER_H

#include "Instruction.h"

enum class Operation {
    NOP,

    ADD,
    SUB,
    AND,
    OR,
    SLT,

    ADDI,
    LW,
    SW,

    BEQ,
    BNE,
    J,

    INVALID
};

struct DecodedInstruction {
    Operation operation;

    uint8_t rs;
    uint8_t rt;
    uint8_t rd;

    int16_t immediate;
    uint32_t address;

    bool usesImmediate;
};

DecodedInstruction decodeInstruction(const Instruction& instruction);

#endif