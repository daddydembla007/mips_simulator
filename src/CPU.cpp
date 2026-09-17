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

void CPU::decodeStage() {

    // --------------------------------------------------------
    // Instruction Decode (ID) stage
    // --------------------------------------------------------
    //
    // The instruction was fetched during the previous cycle
    // and is currently sitting inside the IF/ID pipeline
    // register.
    //
    // The ID stage does three main things:
    //
    // 1. Convert the raw 32-bit instruction into its fields.
    // 2. Determine what operation the instruction represents.
    // 3. Read the required values from the register file.
    //
    // The results are then stored in ID/EX.
    // --------------------------------------------------------

    // If IF/ID does not contain a valid instruction,
    // there is nothing to decode.
    if (!if_id.valid) {

        id_ex.valid = false;

        return;
    }

    // --------------------------------------------------------
    // Step 1: Decode the raw instruction fields
    // --------------------------------------------------------

    // Create an Instruction object.
    //
    // Its constructor automatically extracts:
    // opcode, rs, rt, rd, immediate, etc.
    Instruction instruction(if_id.instruction);

    // Now determine the actual operation:
    //
    // ADD, SUB, ADDI, LW, SW, BEQ, etc.
    DecodedInstruction decoded =
        decodeInstruction(instruction);

    // --------------------------------------------------------
    // Step 2: Read the register file
    // --------------------------------------------------------

    uint32_t readData1 =
        readRegister(decoded.rs);

    uint32_t readData2 =
        readRegister(decoded.rt);

    // --------------------------------------------------------
    // Step 3: Sign-extend the 16-bit immediate
    // --------------------------------------------------------
    //
    // Example:
    //
    // immediate = 10
    //       ↓
    // 00000000 00000000 00000000 00001010
    //
    // immediate = -5
    //       ↓
    // 11111111 11111111 11111111 11111011
    //
    // This gives the EX stage a proper 32-bit value.
    // --------------------------------------------------------

    int32_t immediate =
        static_cast<int32_t>(decoded.immediate);

    // --------------------------------------------------------
    // Step 4: Put everything into ID/EX
    // --------------------------------------------------------

    id_ex.valid = true;

    // Keep the PC associated with this instruction.
    id_ex.pc = if_id.pc;

    // Values read from registers.
    id_ex.readData1 = readData1;
    id_ex.readData2 = readData2;

    // Sign-extended immediate.
    id_ex.immediate = immediate;

    // Register numbers.
    id_ex.rs = decoded.rs;
    id_ex.rt = decoded.rt;
    id_ex.rd = decoded.rd;

    // Operation determined by the decoder.
    id_ex.operation = decoded.operation;
}

void CPU::executeStage() {

    // --------------------------------------------------------
    // Execute (EX) stage
    // --------------------------------------------------------
    //
    // The ID stage has placed all required information into
    // ID/EX.
    //
    // EX is responsible for:
    //
    //   1. Selecting the correct ALU operands
    //   2. Performing the ALU operation
    //   3. Passing the result to EX/MEM
    //
    // For R-type:
    //
    //     ADD $t0, $t1, $t2
    //     ALU = $t1 + $t2
    //
    // For immediate instructions:
    //
    //     ADDI $t0, $t1, 5
    //     ALU = $t1 + 5
    //
    // LW/SW work similarly:
    //
    //     address = base register + offset
    // --------------------------------------------------------

    // Nothing to execute if ID/EX is empty.
    if (!id_ex.valid) {

        ex_mem.valid = false;

        return;
    }


    // --------------------------------------------------------
    // Operand 1
    // --------------------------------------------------------
    //
    // For all instructions currently supported,
    // the first ALU operand comes from rs.
    // --------------------------------------------------------

    uint32_t operand1 =
        id_ex.readData1;


    // --------------------------------------------------------
    // Operand 2
    // --------------------------------------------------------
    //
    // R-type instructions use the value from rt.
    //
    // Immediate instructions use the sign-extended
    // immediate instead.
    // --------------------------------------------------------

    uint32_t operand2;

    if (id_ex.operation == Operation::ADDI ||
        id_ex.operation == Operation::LW ||
        id_ex.operation == Operation::SW) {

        // Immediate-based instruction
        operand2 =
            static_cast<uint32_t>(id_ex.immediate);

    } else {

        // R-type instruction
        operand2 =
            id_ex.readData2;
    }


  // --------------------------------------------------------
// Select the actual ALU operation.
//
// LW and SW are not themselves ALU operations.
// Their EX-stage job is to calculate:
//
//     base address + offset
//
// Therefore, the ALU performs ADD for both LW and SW.
// --------------------------------------------------------

Operation aluOperation = id_ex.operation;

if (id_ex.operation == Operation::LW ||
    id_ex.operation == Operation::SW) {

    aluOperation = Operation::ADD;
}

uint32_t result =
    ALU::execute(
        aluOperation,
        operand1,
        operand2
    );

    // --------------------------------------------------------
    // Store the result in EX/MEM
    // --------------------------------------------------------

    ex_mem.valid = true;

    // ALU result is either:
    //
    //   arithmetic result
    //          OR
    //
    //   effective memory address
    //
    ex_mem.aluResult = result;


    // --------------------------------------------------------
    // Store data for SW
    // --------------------------------------------------------
    //
    // IMPORTANT:
    //
    // For SW:
    //
    //     sw $t0, 4($t1)
    //
    // readData2 contains the value of $t0.
    //
    // We need to carry this value through EX/MEM
    // because MEM will perform the actual store.
    // --------------------------------------------------------

    ex_mem.storeData =
        id_ex.readData2;


    // --------------------------------------------------------
    // Destination register
    // --------------------------------------------------------
    //
    // For R-type:
    //
    //     destination = rd
    //
    // For ADDI/LW:
    //
    //     destination = rt
    //
    // SW has no destination register.
    // --------------------------------------------------------

    if (id_ex.operation == Operation::ADDI ||
        id_ex.operation == Operation::LW) {

        ex_mem.destination =
            id_ex.rt;

    } else {

        ex_mem.destination =
            id_ex.rd;
    }


    // Pass the operation to the MEM stage.
    ex_mem.operation =
        id_ex.operation;
}

void CPU::memoryStage() {

    // --------------------------------------------------------
    // Memory Access (MEM) stage
    // --------------------------------------------------------
    //
    // The EX stage has already calculated the ALU result.
    //
    // For normal arithmetic instructions:
    //
    //     ADD, SUB, AND, OR, SLT
    //
    // the ALU result simply passes through this stage.
    //
    // For memory instructions:
    //
    //     LW -> read from memory
    //     SW -> write to memory
    //
    // --------------------------------------------------------

    // If EX/MEM doesn't contain a valid instruction,
    // there is nothing to process.
    if (!ex_mem.valid) {

        mem_wb.valid = false;

        return;
    }

    // --------------------------------------------------------
    // Default: pass the ALU result forward.
    // --------------------------------------------------------

    mem_wb.valid = true;

    mem_wb.aluResult = ex_mem.aluResult;

    mem_wb.destination = ex_mem.destination;

    mem_wb.operation = ex_mem.operation;


    // --------------------------------------------------------
    // LW: Load Word
    // --------------------------------------------------------
    //
    // EX calculated:
    //
    //     effective address = base + offset
    //
    // That address is stored in aluResult.
    //
    // Now MEM reads the actual data.
    // --------------------------------------------------------

    if (ex_mem.operation == Operation::LW) {

        mem_wb.memoryData =
            readMemoryWord(ex_mem.aluResult);
    }


    // --------------------------------------------------------
    // SW: Store Word
    // --------------------------------------------------------
    //
    // EX calculated the memory address.
    //
    // MEM now writes storeData into that address.
    // --------------------------------------------------------

    else if (ex_mem.operation == Operation::SW) {

        writeMemoryWord(
            ex_mem.aluResult,
            ex_mem.storeData
        );
    }
}

void CPU::writeBackStage() {

    // --------------------------------------------------------
    // Write Back (WB) stage
    // --------------------------------------------------------
    //
    // The MEM stage has placed the final result into MEM/WB.
    //
    // Now we decide what value should be written back into
    // the destination register.
    //
    // Arithmetic instructions:
    //
    //     ADD, SUB, AND, OR, SLT, ADDI
    //
    // use the ALU result.
    //
    // LW uses the value read from memory.
    // --------------------------------------------------------

    // Nothing to write back if MEM/WB is empty.
    if (!mem_wb.valid) {
        return;
    }

    // --------------------------------------------------------
    // LW
    // --------------------------------------------------------
    //
    // For:
    //
    //     lw $t0, 4($t1)
    //
    // the value that needs to go into $t0 is the
    // memoryData field.
    // --------------------------------------------------------

    if (mem_wb.operation == Operation::LW) {

        writeRegister(
            mem_wb.destination,
            mem_wb.memoryData
        );
    }

    // --------------------------------------------------------
    // Arithmetic / immediate instructions
    // --------------------------------------------------------
    //
    // These instructions use the ALU result.
    // --------------------------------------------------------

    else if (mem_wb.operation == Operation::ADD ||
             mem_wb.operation == Operation::SUB ||
             mem_wb.operation == Operation::AND ||
             mem_wb.operation == Operation::OR ||
             mem_wb.operation == Operation::SLT ||
             mem_wb.operation == Operation::ADDI) {

        writeRegister(
            mem_wb.destination,
            mem_wb.aluResult
        );
    }

    // --------------------------------------------------------
    // SW
    // --------------------------------------------------------
    //
    // SW does not write anything to a register.
    //
    // Its work was already completed during MEM.
    // --------------------------------------------------------

    else if (mem_wb.operation == Operation::SW) {

        // Nothing to do.
    }
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