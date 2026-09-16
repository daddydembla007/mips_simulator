#include "CPU.h"
#include "ALU.h"

CPU::CPU() {
    reset();
}

void CPU::reset() {

    // Program execution starts at address 0
    PC = 0;

    // Initially, all 32 registers contain 0
    for (int i = 0; i < 32; i++) {
        registers[i] = 0;
    }

    // No instructions are loaded initially
    instructionMemory.clear();

    // Allocate 1 KB of byte-addressable data memory
    // Every byte is initialized to 0
    dataMemory.resize(1024, 0);
}


uint32_t CPU::readRegister(uint8_t index) const {

    // MIPS has registers numbered from 0 to 31
    if (index >= 32) {
        return 0;
    }

    return registers[index];
}


void CPU::writeRegister(uint8_t index, uint32_t value) {

    // Register $zero (R0) is hardwired to 0.
    // Therefore, writes to R0 are ignored.
    if (index == 0) {
        return;
    }

    if (index < 32) {
        registers[index] = value;
    }
}


uint32_t CPU::readMemoryWord(uint32_t address) const {

    // A 32-bit word occupies 4 consecutive bytes.
    // Make sure all four bytes are inside our memory.
    if (address + 3 >= dataMemory.size()) {
        return 0;
    }

    /*
        Our simulated memory uses big-endian ordering:

        address     -> bits 31-24
        address + 1 -> bits 23-16
        address + 2 -> bits 15-8
        address + 3 -> bits 7-0
    */

    uint32_t value = 0;

    value |= static_cast<uint32_t>(dataMemory[address]) << 24;
    value |= static_cast<uint32_t>(dataMemory[address + 1]) << 16;
    value |= static_cast<uint32_t>(dataMemory[address + 2]) << 8;
    value |= static_cast<uint32_t>(dataMemory[address + 3]);

    return value;
}


void CPU::writeMemoryWord(uint32_t address, uint32_t value) {

    // A 32-bit word requires 4 bytes of memory
    if (address + 3 >= dataMemory.size()) {
        return;
    }

    // Store the most significant byte first
    dataMemory[address] =
        static_cast<uint8_t>((value >> 24) & 0xFF);

    dataMemory[address + 1] =
        static_cast<uint8_t>((value >> 16) & 0xFF);

    dataMemory[address + 2] =
        static_cast<uint8_t>((value >> 8) & 0xFF);

    dataMemory[address + 3] =
        static_cast<uint8_t>(value & 0xFF);
}

void CPU::fetchStage() {
      // --------------------------------------------------------
    // Instruction Fetch (IF) stage
    // --------------------------------------------------------
    //
    // The PC contains the address of the instruction we want
    // to fetch.
    //
    // Since every MIPS32 instruction is 4 bytes:
    //
    //     instruction 0 -> address 0
    //     instruction 1 -> address 4
    //     instruction 2 -> address 8
    //     ...
    //
    // Therefore, after fetching an instruction, PC increases
    // by 4.
    // --------------------------------------------------------

    // Check whether PC points to a valid instruction.
    //
    // PC is a byte address, while instructionMemory is an
    // array/vector where each element represents one
    // 32-bit instruction.
    //
    // Therefore:
    //
    //     instruction index = PC / 4
    //
    // Implementation for the Instruction Fetch stage
    uint32_t instructionIndex = PC / 4;

    if (instructionIndex >= instructionMemory.size()) {

        // There is no instruction at this address.
        //
        // For now, mark the pipeline slot as invalid.
        // This will later allow the pipeline to naturally
        // become empty after the program finishes.
        if_id.valid = false;

        return;
    }

    // --------------------------------------------------------
    // Fetch the instruction
    // --------------------------------------------------------

    uint32_t instruction =
        instructionMemory[instructionIndex];

    // --------------------------------------------------------
    // Put the fetched instruction into the IF/ID register.
    // --------------------------------------------------------

    if_id.valid = true;

    // Save the PC belonging to this instruction.
    if_id.pc = PC;

    // Save the actual 32-bit instruction.
    if_id.instruction = instruction;

    // --------------------------------------------------------
    // Move PC to the next instruction.
    // --------------------------------------------------------

    PC += 4;
}

void CPU::execute(const DecodedInstruction& instruction) {

    switch (instruction.operation) {

        // --------------------------------
        // R-type arithmetic/logic
        // --------------------------------

        case Operation::ADD: {

            // ADD: rd = rs + rt
            uint32_t operand1 =
                readRegister(instruction.rs);

            uint32_t operand2 =
                readRegister(instruction.rt);

            uint32_t result =
                ALU::execute(
                    Operation::ADD,
                    operand1,
                    operand2
                );

            writeRegister(instruction.rd, result);

            break;
        }


        case Operation::SUB: {

            // SUB: rd = rs - rt
            uint32_t operand1 =
                readRegister(instruction.rs);

            uint32_t operand2 =
                readRegister(instruction.rt);

            uint32_t result =
                ALU::execute(
                    Operation::SUB,
                    operand1,
                    operand2
                );

            writeRegister(instruction.rd, result);

            break;
        }


        case Operation::AND: {

            // AND: rd = rs & rt
            uint32_t operand1 =
                readRegister(instruction.rs);

            uint32_t operand2 =
                readRegister(instruction.rt);

            uint32_t result =
                ALU::execute(
                    Operation::AND,
                    operand1,
                    operand2
                );

            writeRegister(instruction.rd, result);

            break;
        }


        case Operation::OR: {

            // OR: rd = rs | rt
            uint32_t operand1 =
                readRegister(instruction.rs);

            uint32_t operand2 =
                readRegister(instruction.rt);

            uint32_t result =
                ALU::execute(
                    Operation::OR,
                    operand1,
                    operand2
                );

            writeRegister(instruction.rd, result);

            break;
        }


        case Operation::SLT: {

            // SLT: rd = (rs < rt) ? 1 : 0
            uint32_t operand1 =
                readRegister(instruction.rs);

            uint32_t operand2 =
                readRegister(instruction.rt);

            uint32_t result =
                ALU::execute(
                    Operation::SLT,
                    operand1,
                    operand2
                );

            writeRegister(instruction.rd, result);

            break;
        }


        // --------------------------------
        // Immediate instruction
        // --------------------------------

        case Operation::ADDI: {

            // ADDI: rt = rs + sign-extended immediate
            uint32_t operand1 =
                readRegister(instruction.rs);

            // Convert the signed 16-bit immediate
            // into a 32-bit signed value
            int32_t immediate =
                static_cast<int32_t>(instruction.immediate);

            uint32_t result =
                ALU::execute(
                    Operation::ADDI,
                    operand1,
                    static_cast<uint32_t>(immediate)
                );

            // ADDI stores its result in rt
            writeRegister(instruction.rt, result);

            break;
        }


        // --------------------------------
        // Load Word
        // --------------------------------

        case Operation::LW: {

            // LW:
            // rt = Memory[rs + immediate]

            uint32_t base =
                readRegister(instruction.rs);

            int32_t offset =
                static_cast<int32_t>(instruction.immediate);

            // Calculate effective memory address
            uint32_t address =
                base + static_cast<uint32_t>(offset);

            // Read a 32-bit word from memory
            uint32_t value =
                readMemoryWord(address);

            // Store loaded value in rt
            writeRegister(instruction.rt, value);

            break;
        }


        // --------------------------------
        // Store Word
        // --------------------------------

        case Operation::SW: {

            // SW:
            // Memory[rs + immediate] = rt

            uint32_t base =
                readRegister(instruction.rs);

            int32_t offset =
                static_cast<int32_t>(instruction.immediate);

            // Calculate effective memory address
            uint32_t address =
                base + static_cast<uint32_t>(offset);

            // Get value from rt
            uint32_t value =
                readRegister(instruction.rt);

            // Store the value into memory
            writeMemoryWord(address, value);

            break;
        }


        // We will implement branches and jumps
        // properly when we build the pipeline.
        case Operation::BEQ:
        case Operation::BNE:
        case Operation::J:

            break;


        // Invalid or unsupported instruction
        case Operation::INVALID:
        case Operation::NOP:

            break;
    }
}