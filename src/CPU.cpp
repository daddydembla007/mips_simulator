#include "CPU.h"
#include "ALU.h"
#include <iostream>
CPU::CPU() {
    reset();
}

void CPU::reset() {

    PC = 0;

    // --------------------------------------------------------
    // Reset registers
    // --------------------------------------------------------

    for (int i = 0; i < 32; i++) {
        registers[i] = 0;
    }

    // --------------------------------------------------------
    // Register $zero is always zero.
    // --------------------------------------------------------

    registers[0] = 0;

    // --------------------------------------------------------
    // Reset cycle counter.
    // --------------------------------------------------------

    cycle = 0;

    // --------------------------------------------------------
    // Reset control-flow state.
    // --------------------------------------------------------

    branchTaken = false;

    // --------------------------------------------------------
    // Reset statistics.
    // --------------------------------------------------------

    instructionsRetired = 0;
    stallCount = 0;
    flushCount = 0;
    flushedInstructionCount = 0;

    lastEvent = "RESET";

    // --------------------------------------------------------
    // Reset instruction memory.
    // --------------------------------------------------------

    instructionMemory.clear();

    // --------------------------------------------------------
    // Reset data memory.
    //
    // Our simulator currently has 1 KB of byte-addressable
    // memory.
    // --------------------------------------------------------

    dataMemory.assign(1024, 0);

    // --------------------------------------------------------
    // Reset pipeline registers.
    // --------------------------------------------------------

    if_id = IF_ID{};
    id_ex = ID_EX{};
    ex_mem = EX_MEM{};
    mem_wb = MEM_WB{};

    next_if_id = IF_ID{};
    next_id_ex = ID_EX{};
    next_ex_mem = EX_MEM{};
    next_mem_wb = MEM_WB{};
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
    // Fetch the instruction pointed to by PC.
    //
    // IMPORTANT:
    // We write into NEXT IF/ID, not the current IF/ID.
    //
    // The current IF/ID belongs to the instruction that is
    // already being processed by the ID stage this cycle.
    // --------------------------------------------------------

    uint32_t instructionIndex = PC / 4;

    // --------------------------------------------------------
    // Check whether PC points to a valid instruction.
    // --------------------------------------------------------

    if (instructionIndex >= instructionMemory.size()) {

        // No instruction to fetch.
        //
        // The next IF/ID register will therefore be empty.
        next_if_id.valid = false;

        return;
    }

    // --------------------------------------------------------
    // Fetch instruction from instruction memory.
    // --------------------------------------------------------

    uint32_t instruction =
        instructionMemory[instructionIndex];

    // --------------------------------------------------------
    // Write fetched instruction into NEXT IF/ID.
    // --------------------------------------------------------

    next_if_id.valid = true;

    // PC associated with this instruction.
    next_if_id.pc = PC;

    // Actual 32-bit instruction.
    next_if_id.instruction = instruction;

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
    // CURRENT:
    //
    //     IF/ID → ID
    //
    // NEXT:
    //
    //     ID → next ID/EX
    //
    // --------------------------------------------------------

    // If there is no valid instruction in IF/ID,
    // insert an empty entry into the next pipeline stage.
    if (!if_id.valid) {

        next_id_ex.valid = false;

        return;
    }


    // --------------------------------------------------------
    // Step 1: Decode the raw instruction
    // --------------------------------------------------------

    Instruction instruction(if_id.instruction);

    DecodedInstruction decoded =
        decodeInstruction(instruction);


    // --------------------------------------------------------
    // Step 2: Read registers
    // --------------------------------------------------------

    uint32_t readData1 =
        readRegister(decoded.rs);

    uint32_t readData2 =
        readRegister(decoded.rt);


    // --------------------------------------------------------
    // Step 3: Sign-extend immediate
    // --------------------------------------------------------

    int32_t immediate =
        static_cast<int32_t>(decoded.immediate);


    // --------------------------------------------------------
    // Step 4: Fill NEXT ID/EX
    // --------------------------------------------------------

    next_id_ex.valid = true;

    next_id_ex.pc =
        if_id.pc;

    next_id_ex.readData1 =
        readData1;

    next_id_ex.readData2 =
        readData2;

    next_id_ex.immediate =
        immediate;

    next_id_ex.rs =
        decoded.rs;

    next_id_ex.rt =
        decoded.rt;

    next_id_ex.rd =
        decoded.rd;

    next_id_ex.operation =
        decoded.operation;


    // --------------------------------------------------------
    // Step 5: Determine whether this is a branch
    // --------------------------------------------------------

    next_id_ex.isBranch = false;

    next_id_ex.branchNotEqual = false;

    next_id_ex.branchTarget = 0;

    // --------------------------------------------------------
// Jump information
// --------------------------------------------------------

next_id_ex.isJump = false;

next_id_ex.jumpTarget = 0;


    if (decoded.operation == Operation::BEQ ||
        decoded.operation == Operation::BNE) {

        // This instruction is a branch.
        next_id_ex.isBranch = true;

        // BNE requires the "not equal" condition.
        if (decoded.operation == Operation::BNE) {

            next_id_ex.branchNotEqual = true;
        }


        // ----------------------------------------------------
        // Calculate branch target.
        //
        //     target =
        //         PC + 4 + (immediate << 2)
        //
        // The immediate represents the number of
        // instructions to move, not the number of bytes.
        // Therefore we multiply it by 4.
        // ----------------------------------------------------

        next_id_ex.branchTarget =
            if_id.pc
            + 4
            + (static_cast<int32_t>(immediate << 2));
    }

    // --------------------------------------------------------
// JUMP
// --------------------------------------------------------
//
// MIPS J instruction:
//
//     target = (PC + 4)[31:28] | (address << 2)
//
// The instruction contains a 26-bit address field.
// Since instructions are word-aligned, we shift it left
// by 2 to convert it into a byte address.
// --------------------------------------------------------

if (decoded.operation == Operation::J) {

    next_id_ex.isJump = true;

    // Upper 4 bits come from PC + 4.
    uint32_t upperBits =
        (if_id.pc + 4) & 0xF0000000;

    // Lower 28 bits come from the 26-bit instruction
    // address after shifting left by 2.
    uint32_t lowerBits =
        (decoded.address << 2);

    next_id_ex.jumpTarget =
        upperBits | lowerBits;
}
}

void CPU::executeStage() {

    // --------------------------------------------------------
    // EX Stage
    // --------------------------------------------------------

    branchTaken = false;

    // --------------------------------------------------------
    // Bubble
    // --------------------------------------------------------

    if (!id_ex.valid) {
        next_ex_mem.valid = false;
        return;
    }

    // --------------------------------------------------------
    // NOP
    // --------------------------------------------------------

    if (id_ex.operation == Operation::NOP) {
        next_ex_mem.valid = false;
        return;
    }

    // ========================================================
    // JUMP
    // ========================================================

   if (id_ex.operation == Operation::J) {
    // Jump is resolved in EX stage, so it retires here.
    instructionsRetired++;

    branchTaken = true;
    PC = id_ex.jumpTarget;

    next_ex_mem.valid = false;
    return;
}

    // ========================================================
    // BRANCH
    // ========================================================

    if (id_ex.operation == Operation::BEQ ||
        id_ex.operation == Operation::BNE) {

        uint32_t value1 = id_ex.readData1;
        uint32_t value2 = id_ex.readData2;

        // ----------------------------------------------------
        // First check the result produced by the current
        // MEM stage.
        //
        // This is especially important for LW.
        // ----------------------------------------------------

        if (next_mem_wb.valid &&
            next_mem_wb.destination != 0 &&
            next_mem_wb.destination == id_ex.rs) {

            if (next_mem_wb.operation == Operation::LW) {
                value1 = next_mem_wb.memoryData;
            }
            else {
                value1 = next_mem_wb.aluResult;
            }
        }

        else if (ex_mem.valid &&
                 ex_mem.destination != 0 &&
                 ex_mem.destination == id_ex.rs &&
                 ex_mem.operation != Operation::SW &&
                 ex_mem.operation != Operation::LW) {

            value1 = ex_mem.aluResult;
        }

        else if (mem_wb.valid &&
                 mem_wb.destination != 0 &&
                 mem_wb.destination == id_ex.rs) {

            if (mem_wb.operation == Operation::LW) {
                value1 = mem_wb.memoryData;
            }
            else {
                value1 = mem_wb.aluResult;
            }
        }

        // ----------------------------------------------------
        // Second branch operand
        // ----------------------------------------------------

        if (next_mem_wb.valid &&
            next_mem_wb.destination != 0 &&
            next_mem_wb.destination == id_ex.rt) {

            if (next_mem_wb.operation == Operation::LW) {
                value2 = next_mem_wb.memoryData;
            }
            else {
                value2 = next_mem_wb.aluResult;
            }
        }

        else if (ex_mem.valid &&
                 ex_mem.destination != 0 &&
                 ex_mem.destination == id_ex.rt &&
                 ex_mem.operation != Operation::SW &&
                 ex_mem.operation != Operation::LW) {

            value2 = ex_mem.aluResult;
        }

        else if (mem_wb.valid &&
                 mem_wb.destination != 0 &&
                 mem_wb.destination == id_ex.rt) {

            if (mem_wb.operation == Operation::LW) {
                value2 = mem_wb.memoryData;
            }
            else {
                value2 = mem_wb.aluResult;
            }
        }

        // ----------------------------------------------------
        // Check branch condition
        // ----------------------------------------------------

        bool taken;

        if (id_ex.operation == Operation::BEQ) {
            taken = (value1 == value2);
        }
        else {
            taken = (value1 != value2);
        }

        // ----------------------------------------------------
        // Redirect PC
        // ----------------------------------------------------

       if (taken) {
    branchTaken = true;
    PC = id_ex.branchTarget;
}

// Branch instruction itself has completed EX,
// regardless of whether the branch was taken.
instructionsRetired++;

next_ex_mem.valid = false;
return;
    }

    // ========================================================
    // NORMAL INSTRUCTIONS
    // ========================================================

    // --------------------------------------------------------
    // Operand 1
    // --------------------------------------------------------

    uint32_t operand1 = id_ex.readData1;

    // --------------------------------------------------------
    // IMPORTANT:
    //
    // Check next_mem_wb FIRST.
    //
    // If the current MEM stage is processing:
    //
    //     LW $t0, 0($t1)
    //
    // then next_mem_wb.memoryData contains the actual
    // loaded value.
    // --------------------------------------------------------

    if (next_mem_wb.valid &&
        next_mem_wb.destination != 0 &&
        next_mem_wb.destination == id_ex.rs) {

        if (next_mem_wb.operation == Operation::LW) {

            // Forward the VALUE loaded from memory.
            operand1 = next_mem_wb.memoryData;
        }
        else {

            operand1 = next_mem_wb.aluResult;
        }
    }

    // --------------------------------------------------------
    // Otherwise check EX/MEM.
    //
    // Do NOT forward EX/MEM for LW because its ALU result
    // is the memory address, not the loaded value.
    // --------------------------------------------------------

    else if (ex_mem.valid &&
             ex_mem.destination != 0 &&
             ex_mem.destination == id_ex.rs &&
             ex_mem.operation != Operation::SW &&
             ex_mem.operation != Operation::LW) {

        operand1 = ex_mem.aluResult;
    }

    // --------------------------------------------------------
    // Finally check old MEM/WB.
    // --------------------------------------------------------

    else if (mem_wb.valid &&
             mem_wb.destination != 0 &&
             mem_wb.destination == id_ex.rs) {

        if (mem_wb.operation == Operation::LW) {
            operand1 = mem_wb.memoryData;
        }
        else {
            operand1 = mem_wb.aluResult;
        }
    }

    // ========================================================
    // Operand 2
    // ========================================================

    uint32_t operand2;

    // --------------------------------------------------------
    // Immediate instructions
    // --------------------------------------------------------

    if (id_ex.operation == Operation::ADDI ||
        id_ex.operation == Operation::LW ||
        id_ex.operation == Operation::SW) {

        operand2 =
            static_cast<uint32_t>(id_ex.immediate);
    }

    // --------------------------------------------------------
    // R-type instructions
    // --------------------------------------------------------

    else {

        operand2 = id_ex.readData2;

        // ----------------------------------------------------
        // Current MEM stage
        // ----------------------------------------------------

        if (next_mem_wb.valid &&
            next_mem_wb.destination != 0 &&
            next_mem_wb.destination == id_ex.rt) {

            if (next_mem_wb.operation == Operation::LW) {
                operand2 = next_mem_wb.memoryData;
            }
            else {
                operand2 = next_mem_wb.aluResult;
            }
        }

        // ----------------------------------------------------
        // EX/MEM
        // ----------------------------------------------------

        else if (ex_mem.valid &&
                 ex_mem.destination != 0 &&
                 ex_mem.destination == id_ex.rt &&
                 ex_mem.operation != Operation::SW &&
                 ex_mem.operation != Operation::LW) {

            operand2 = ex_mem.aluResult;
        }

        // ----------------------------------------------------
        // MEM/WB
        // ----------------------------------------------------

        else if (mem_wb.valid &&
                 mem_wb.destination != 0 &&
                 mem_wb.destination == id_ex.rt) {

            if (mem_wb.operation == Operation::LW) {
                operand2 = mem_wb.memoryData;
            }
            else {
                operand2 = mem_wb.aluResult;
            }
        }
    }

    // ========================================================
    // Determine ALU operation
    // ========================================================

    Operation aluOperation = id_ex.operation;

    // LW/SW calculate:
    //
    //     base address + offset
    //

    if (id_ex.operation == Operation::LW ||
        id_ex.operation == Operation::SW) {

        aluOperation = Operation::ADD;
    }

    // ========================================================
    // ALU
    // ========================================================

    uint32_t result =
        ALU::execute(
            aluOperation,
            operand1,
            operand2
        );

    // ========================================================
    // EX/MEM
    // ========================================================

    next_ex_mem.valid = true;

    next_ex_mem.aluResult = result;

    // --------------------------------------------------------
    // Store data
    // --------------------------------------------------------

    next_ex_mem.storeData = id_ex.readData2;

    // If this is SW, the value being stored may also need
    // forwarding.
    //

    if (id_ex.operation == Operation::SW) {

        if (next_mem_wb.valid &&
            next_mem_wb.destination != 0 &&
            next_mem_wb.destination == id_ex.rt) {

            if (next_mem_wb.operation == Operation::LW) {
                next_ex_mem.storeData =
                    next_mem_wb.memoryData;
            }
            else {
                next_ex_mem.storeData =
                    next_mem_wb.aluResult;
            }
        }

        else if (ex_mem.valid &&
                 ex_mem.destination != 0 &&
                 ex_mem.destination == id_ex.rt &&
                 ex_mem.operation != Operation::SW &&
                 ex_mem.operation != Operation::LW) {

            next_ex_mem.storeData =
                ex_mem.aluResult;
        }

        else if (mem_wb.valid &&
                 mem_wb.destination != 0 &&
                 mem_wb.destination == id_ex.rt) {

            if (mem_wb.operation == Operation::LW) {
                next_ex_mem.storeData =
                    mem_wb.memoryData;
            }
            else {
                next_ex_mem.storeData =
                    mem_wb.aluResult;
            }
        }
    }

    // ========================================================
    // Destination register
    // ========================================================

    if (id_ex.operation == Operation::ADDI ||
        id_ex.operation == Operation::LW) {

        next_ex_mem.destination = id_ex.rt;
    }
    else {

        next_ex_mem.destination = id_ex.rd;
    }

    // ========================================================
    // Pass operation to EX/MEM
    // ========================================================

    next_ex_mem.operation = id_ex.operation;
}

void CPU::memoryStage() {

    // --------------------------------------------------------
    // Memory Access (MEM) stage
    // --------------------------------------------------------
    //
    // CURRENT:
    //
    //     EX/MEM → MEM
    //
    // NEXT:
    //
    //     MEM → next MEM/WB
    //
    // Arithmetic instructions simply pass their ALU result
    // through this stage.
    //
    // LW reads memory.
    //
    // SW writes memory.
    // --------------------------------------------------------

    // If EX/MEM is empty, there is nothing to process.
    if (!ex_mem.valid) {

        next_mem_wb.valid = false;

        return;
    }


    // --------------------------------------------------------
    // Default values
    // --------------------------------------------------------

    next_mem_wb.valid = true;

    // Pass the ALU result forward.
    next_mem_wb.aluResult =
        ex_mem.aluResult;

    // Pass the destination register forward.
    next_mem_wb.destination =
        ex_mem.destination;

    // Pass the operation forward.
    next_mem_wb.operation =
        ex_mem.operation;


    // --------------------------------------------------------
    // LW — Load Word
    // --------------------------------------------------------
    //
    // EX calculated the effective address:
    //
    //     base + offset
    //
    // Now MEM reads the value from that address.
    // --------------------------------------------------------

    if (ex_mem.operation == Operation::LW) {

        next_mem_wb.memoryData =
            readMemoryWord(ex_mem.aluResult);
    }


    // --------------------------------------------------------
    // SW — Store Word
    // --------------------------------------------------------
    //
    // EX calculated the effective address.
    //
    // MEM performs the actual memory write.
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
    // Nothing to write back.
    // --------------------------------------------------------

    if (!mem_wb.valid) {
        return;
    }

    // --------------------------------------------------------
    // Count this instruction as retired.
    //
    // At this point the instruction has successfully reached
    // the final stage of the pipeline.
    // --------------------------------------------------------

    if (mem_wb.operation != Operation::NOP) {
        instructionsRetired++;
    }

    // --------------------------------------------------------
    // LW
    // --------------------------------------------------------

    if (mem_wb.operation == Operation::LW) {

        writeRegister(
            mem_wb.destination,
            mem_wb.memoryData
        );
    }

    // --------------------------------------------------------
    // ALU instructions
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
    //
    // Memory write already happened in MEM.
    // Nothing happens in WB.
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

void CPU::step() {

    // ========================================================
    // Start a new clock cycle
    // ========================================================

    cycle++;

    // By default, assume nothing special happened.
    lastEvent = "NORMAL";

    // ========================================================
    // Pipeline stages
    //
    // We execute from the back of the pipeline toward the
    // front so that each stage sees the pipeline registers
    // from the beginning of this clock cycle.
    // ========================================================

    writeBackStage();
    memoryStage();
    executeStage();


    // ========================================================
    // CONTROL HAZARD
    //
    // A taken BEQ/BNE/J changes the PC.
    //
    // Instructions following the branch/jump that are already
    // in the pipeline belong to the wrong execution path.
    //
    // They must therefore be flushed.
    // ========================================================

    if (branchTaken) {

        lastEvent = "FLUSH";

        // Count the control-flow event.
        flushCount++;

        // In our 5-stage model, the instructions in IF/ID
        // and ID/EX are the wrong-path instructions.
        //
        // Therefore two pipeline slots are flushed.
        flushedInstructionCount += 2;

        // Flush IF/ID.
        next_if_id.valid = false;

        // Flush ID/EX.
        next_id_ex.valid = false;
    }

    else {

        // ====================================================
        // DATA HAZARD
        //
        // Example:
        //
        //     LW  $t0, 0($t1)
        //     ADD $t3, $t0, $t2
        //
        // The ADD cannot immediately use the value loaded
        // by the LW.
        //
        // Therefore we insert one bubble.
        // ====================================================

        bool loadUseHazard = hasLoadUseHazard();

        if (loadUseHazard) {

            lastEvent = "STALL";

            // Count the stall.
            stallCount++;

            // ------------------------------------------------
            // Insert a bubble into ID/EX.
            // ------------------------------------------------

            next_id_ex.valid = false;

            // ------------------------------------------------
            // Freeze IF/ID.
            //
            // The dependent instruction stays here and will
            // be decoded on the next cycle.
            // ------------------------------------------------

            next_if_id = if_id;
        }

        else {

            // =================================================
            // No hazard.
            //
            // Normal operation:
            //
            //     ID -> ID/EX
            //     IF -> IF/ID
            // =================================================

            decodeStage();
            fetchStage();
        }
    }


    // ========================================================
    // CLOCK EDGE
    //
    // The "next" pipeline registers now become the current
    // pipeline registers.
    // ========================================================

    if_id = next_if_id;

    id_ex = next_id_ex;

    ex_mem = next_ex_mem;

    mem_wb = next_mem_wb;
}

bool CPU::hasLoadUseHazard() const {

    // --------------------------------------------------------
    // We only have a load-use hazard when:
    //
    //     LW is currently in ID/EX
    //
    // and the instruction in IF/ID needs the value loaded
    // by that LW.
    // --------------------------------------------------------

    if (!id_ex.valid) {
        return false;
    }

    if (id_ex.operation != Operation::LW) {
        return false;
    }

    if (!if_id.valid) {
        return false;
    }

    // --------------------------------------------------------
    // The destination of:
    //
    //     LW rt, offset(rs)
    //
    // is RT.
    // --------------------------------------------------------

    uint8_t loadDestination = id_ex.rt;

    // Register $zero can never cause a real dependency.
    if (loadDestination == 0) {
        return false;
    }

    // --------------------------------------------------------
    // Decode the instruction currently sitting in IF/ID.
    // --------------------------------------------------------

    Instruction instruction(if_id.instruction);

    DecodedInstruction decoded =
        decodeInstruction(instruction);

    // --------------------------------------------------------
    // Determine which registers the current instruction reads.
    //
    // ADD:
    //
    //     ADD rd, rs, rt
    //
    // reads BOTH rs and rt.
    //
    // ADDI:
    //
    //     ADDI rt, rs, immediate
    //
    // reads only rs.
    //
    // LW:
    //
    //     LW rt, offset(rs)
    //
    // reads only rs.
    //
    // SW:
    //
    //     SW rt, offset(rs)
    //
    // reads rs (base address) and rt (data to store).
    //
    // BEQ/BNE:
    //
    //     BEQ rs, rt, offset
    //
    // reads BOTH rs and rt.
    // --------------------------------------------------------
    
    bool usesRs = false;
    bool usesRt = false;

    switch (decoded.operation) {

        // ----------------------------------------------------
        // R-type instructions
        // ----------------------------------------------------

        case Operation::ADD:
        case Operation::SUB:
        case Operation::AND:
        case Operation::OR:
        case Operation::SLT:

            usesRs = true;
            usesRt = true;

            break;

        // ----------------------------------------------------
        // ADDI
        //
        // ADDI rt, rs, immediate
        //
        // Only RS is read.
        // ----------------------------------------------------

        case Operation::ADDI:

            usesRs = true;
            usesRt = false;

            break;

        // ----------------------------------------------------
        // LW
        //
        // LW rt, offset(rs)
        //
        // Only RS is read.
        // ----------------------------------------------------

        case Operation::LW:

            usesRs = true;
            usesRt = false;

            break;

        // ----------------------------------------------------
        // SW
        //
        // SW rt, offset(rs)
        //
        // RS = base address
        // RT = value being stored
        // ----------------------------------------------------

        case Operation::SW:

            usesRs = true;
            usesRt = true;

            break;

        // ----------------------------------------------------
        // Branches
        //
        // BEQ/BNE both read RS and RT.
        // ----------------------------------------------------

        case Operation::BEQ:
        case Operation::BNE:

            usesRs = true;
            usesRt = true;

            break;

        // ----------------------------------------------------
        // Jump
        //
        // J does not read a register.
        // ----------------------------------------------------

        case Operation::J:

            usesRs = false;
            usesRt = false;

            break;

        // ----------------------------------------------------
        // NOP / invalid
        // ----------------------------------------------------

        case Operation::NOP:
        case Operation::INVALID:

            usesRs = false;
            usesRt = false;

            break;
    }

    // --------------------------------------------------------
    // Check RS dependency.
    // --------------------------------------------------------

    if (usesRs &&
        decoded.rs == loadDestination) {

        return true;
    }

    // --------------------------------------------------------
    // Check RT dependency.
    // --------------------------------------------------------

    if (usesRt &&
        decoded.rt == loadDestination) {

        return true;
    }

    // --------------------------------------------------------
    // No dependency.
    // --------------------------------------------------------

    return false;
}