#ifndef PIPELINE_H
#define PIPELINE_H

#include <cstdint>
#include "Decoder.h"


// ============================================================
// IF/ID Pipeline Register
// ============================================================
//
// Stores information produced by the IF (Instruction Fetch)
// stage so that the ID (Instruction Decode) stage can use it
// during the next clock cycle.
//
struct IF_ID {

    // Tells us whether this pipeline slot actually contains
    // a valid instruction.
    bool valid = false;

    // Address of the instruction that was fetched.
    uint32_t pc = 0;

    // The raw 32-bit MIPS instruction.
    uint32_t instruction = 0;
};


// ============================================================
// ID/EX Pipeline Register
// ============================================================
//
// Stores information produced by the ID stage so that the
// EX (Execute) stage can use it during the next clock cycle.
//
struct ID_EX {

    bool valid = false;

    // PC of the instruction.
    uint32_t pc = 0;

    // Values read from the register file.
    uint32_t readData1 = 0;
    uint32_t readData2 = 0;

    // Sign-extended immediate value.
    int32_t immediate = 0;

    // Register numbers used by the instruction.
    uint8_t rs = 0;
    uint8_t rt = 0;
    uint8_t rd = 0;

    // What operation should the ALU / EX stage perform?
    Operation operation = Operation::NOP;
};


// ============================================================
// EX/MEM Pipeline Register
// ============================================================
//
// Stores information produced by the EX stage so that the
// MEM (Memory Access) stage can use it during the next cycle.
//
struct EX_MEM {

    bool valid = false;

    // Result produced by the ALU.
    uint32_t aluResult = 0;

    // Data that needs to be stored for an SW instruction.
    uint32_t storeData = 0;

    // Register that will eventually receive the result.
    uint8_t destination = 0;

    // Operation performed by this instruction.
    Operation operation = Operation::NOP;
};


// ============================================================
// MEM/WB Pipeline Register
// ============================================================
//
// Stores information produced by the MEM stage so that the
// WB (Write Back) stage can use it during the next cycle.
//
struct MEM_WB {

    bool valid = false;

    // Data read from memory for an LW instruction.
    uint32_t memoryData = 0;

    // ALU result, used by arithmetic instructions.
    uint32_t aluResult = 0;

    // Register that will receive the final result.
    uint8_t destination = 0;

    // Operation performed by this instruction.
    Operation operation = Operation::NOP;
};

#endif