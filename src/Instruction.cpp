#include "Instruction.h"

Instruction::Instruction(uint32_t instruction) {
    raw = instruction;
    decode();
}

void Instruction::decode() {

    opcode = (raw >> 26) & 0x3F;

    rs = (raw >> 21) & 0x1F;

    rt = (raw >> 16) & 0x1F;

    rd = (raw >> 11) & 0x1F;

    shamt = (raw >> 6) & 0x1F;

    funct = raw & 0x3F;

    immediate = raw & 0xFFFF;

    address = raw & 0x03FFFFFF;
}