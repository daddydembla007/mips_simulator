#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <vector>
#include "Pipeline.h"
#include "Decoder.h"
#include <string>
class CPU {
public:
    // Program Counter: stores the address of the next instruction
    uint32_t PC;
    // True when the instruction currently in EX
// causes a taken branch.
bool branchTaken;
    // MIPS has 32 general-purpose registers, each 32 bits wide
    uint32_t registers[32];
    // Number of clock cycles executed so far
    uint64_t cycle;
    // Instruction memory: each instruction is 32 bits
    std::vector<uint32_t> instructionMemory;

    // Data memory is byte-addressable
    std::vector<uint8_t> dataMemory;
  // Pipeline register between IF and ID stages
IF_ID if_id;

// Pipeline register between ID and EX stages
ID_EX id_ex;

// Pipeline register between EX and MEM stages
EX_MEM ex_mem;
// Pipeline register between MEM and WB stages
MEM_WB mem_wb;

// --------------------------------------------------------
// Next-state pipeline registers
// --------------------------------------------------------
//
// During a clock cycle:
//
//   current registers → stages → next registers
//
// At the end of the cycle:
//
//   next registers → current registers
//
// This prevents one instruction from accidentally
// travelling through multiple stages during one cycle.
// --------------------------------------------------------

IF_ID next_if_id;
ID_EX next_id_ex;
EX_MEM next_ex_mem;
MEM_WB next_mem_wb;

// ============================================================
// Processor statistics
// ============================================================

// Total number of instructions that completed execution.
uint64_t instructionsRetired = 0;

// Number of cycles in which a load-use stall was inserted.
uint64_t stallCount = 0;

// Number of taken branches/jumps that caused a pipeline flush.
uint64_t flushCount = 0;

// Number of wrong-path instructions removed because of flushes.
uint64_t flushedInstructionCount = 0;

// Description of what happened during the current cycle.
// Used by the simulator visualization.
std::string lastEvent;
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

    void fetchStage();
    void decodeStage();
    void executeStage();
    void memoryStage();
    void writeBackStage();
    void step(); // advance processor by one clock cycle
    // Detect a load-use data hazard between ID and EX
    bool hasLoadUseHazard() const;
    
};

#endif