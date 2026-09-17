#include "CPU.h"
#include "ALU.h"

CPU::CPU() {
    reset();
}

void CPU::reset() {

    // Program execution starts at address 0
    PC = 0;
    cycle = 0;
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
    // ID reads the CURRENT IF/ID register.
    //
    // It must NOT modify if_id because IF is simultaneously
    // producing the next instruction.
    //
    // Therefore:
    //
    //     CURRENT IF/ID → ID → NEXT ID/EX
    // --------------------------------------------------------

    // If IF/ID does not contain a valid instruction,
    // there is nothing to decode.
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
    // Step 2: Read the register file
    // --------------------------------------------------------

    uint32_t readData1 =
        readRegister(decoded.rs);

    uint32_t readData2 =
        readRegister(decoded.rt);


    // --------------------------------------------------------
    // Step 3: Sign-extend the immediate
    // --------------------------------------------------------

    int32_t immediate =
        static_cast<int32_t>(decoded.immediate);


    // --------------------------------------------------------
    // Step 4: Write everything into NEXT ID/EX
    // --------------------------------------------------------

    next_id_ex.valid = true;

    // PC belonging to this instruction.
    next_id_ex.pc = if_id.pc;

    // Values read from the register file.
    next_id_ex.readData1 = readData1;
    next_id_ex.readData2 = readData2;

    // Sign-extended immediate.
    next_id_ex.immediate = immediate;

    // Register numbers.
    next_id_ex.rs = decoded.rs;
    next_id_ex.rt = decoded.rt;
    next_id_ex.rd = decoded.rd;

    // Decoded operation.
    next_id_ex.operation = decoded.operation;
}

void CPU::executeStage() {

    // --------------------------------------------------------
    // Execute (EX) stage
    // --------------------------------------------------------
    //
    // CURRENT:
    //
    //     ID/EX → EX
    //
    // NEXT:
    //
    //     EX → next EX/MEM
    //
    // EX calculates the ALU result and determines which
    // register will eventually receive the result.
    // --------------------------------------------------------

    // If there is no valid instruction in ID/EX,
    // EX has nothing to execute.
    if (!id_ex.valid) {

        next_ex_mem.valid = false;

        return;
    }


   // --------------------------------------------------------
// Forwarding for operand 1
// --------------------------------------------------------
//
// Normally, operand1 comes from the value captured during ID.
//
// But if a previous instruction has just produced a result
// for the same register, that result may not have reached
// the register file yet.
//
// In that case, forward the newer value directly.
// --------------------------------------------------------

uint32_t operand1 =
    id_ex.readData1;


// Check EX/MEM first because it contains the most recently
// produced ALU result.
if (ex_mem.valid &&
    ex_mem.destination != 0 &&
    ex_mem.destination == id_ex.rs &&
    ex_mem.operation != Operation::SW &&
    ex_mem.operation != Operation::LW) {

    operand1 =
        ex_mem.aluResult;
}


// If EX/MEM didn't provide the value, check MEM/WB.
else if (mem_wb.valid &&
         mem_wb.destination != 0 &&
         mem_wb.destination == id_ex.rs) {

    // LW gets its value from memory.
    if (mem_wb.operation == Operation::LW) {

        operand1 =
            mem_wb.memoryData;
    }

    // Arithmetic instructions get their value from ALU.
    else {

        operand1 =
            mem_wb.aluResult;
    }
}


   uint32_t operand2;


// --------------------------------------------------------
// Immediate instructions
// --------------------------------------------------------
//
// ADDI, LW and SW use the immediate as their second ALU
// operand, so there is no register value to forward here.
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
//
// The second operand normally comes from rt.
//
// However, it may need forwarding just like operand1.
// --------------------------------------------------------

else {

    operand2 =
        id_ex.readData2;


    // Check the most recent ALU result first.
    if (ex_mem.valid &&
        ex_mem.destination != 0 &&
        ex_mem.destination == id_ex.rt &&
        ex_mem.operation != Operation::SW &&
        ex_mem.operation != Operation::LW) {

        operand2 =
            ex_mem.aluResult;
    }


    // Otherwise check MEM/WB.
    else if (mem_wb.valid &&
             mem_wb.destination != 0 &&
             mem_wb.destination == id_ex.rt) {

        if (mem_wb.operation == Operation::LW) {

            operand2 =
                mem_wb.memoryData;
        }

        else {

            operand2 =
                mem_wb.aluResult;
        }
    }
}

    // --------------------------------------------------------
    // Determine the actual ALU operation.
    // --------------------------------------------------------
    //
    // LW and SW need the ALU to calculate:
    //
    //     base address + offset
    //
    // Therefore, the ALU performs ADD for them.
    // --------------------------------------------------------

    Operation aluOperation =
        id_ex.operation;

    if (id_ex.operation == Operation::LW ||
        id_ex.operation == Operation::SW) {

        aluOperation = Operation::ADD;
    }


    // --------------------------------------------------------
    // Perform ALU operation
    // --------------------------------------------------------

    uint32_t result =
        ALU::execute(
            aluOperation,
            operand1,
            operand2
        );


    // --------------------------------------------------------
    // Write results into NEXT EX/MEM
    // --------------------------------------------------------

    next_ex_mem.valid = true;

    // ALU result.
    //
    // For arithmetic:
    //     actual arithmetic result
    //
    // For LW/SW:
    //     calculated memory address
    next_ex_mem.aluResult =
        result;


    // --------------------------------------------------------
    // Store data
    // --------------------------------------------------------
    //
    // Needed by SW.
    //
    // Example:
    //
    //     sw $t0, 4($t1)
    //
    // readData2 contains the value of $t0.
    // --------------------------------------------------------

    next_ex_mem.storeData =
        id_ex.readData2;


    // --------------------------------------------------------
    // Destination register
    // --------------------------------------------------------
    //
    // R-type:
    //
    //     destination = rd
    //
    // ADDI/LW:
    //
    //     destination = rt
    //
    // SW doesn't actually write a register, but keeping a
    // value here is harmless because WB will ignore SW.
    // --------------------------------------------------------

    if (id_ex.operation == Operation::ADDI ||
        id_ex.operation == Operation::LW) {

        next_ex_mem.destination =
            id_ex.rt;

    } else {

        next_ex_mem.destination =
            id_ex.rd;
    }


    // Pass the instruction type to the next stage.
    next_ex_mem.operation =
        id_ex.operation;
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
    // Write Back (WB) stage
    // --------------------------------------------------------
    //
    // WB reads the CURRENT MEM/WB register.
    //
    // Unlike the other stages, WB does not produce another
    // pipeline register.
    //
    // It writes the final result into the register file.
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
    // MEM has already read the value from memory.
    //
    // That value is now in:
    //
    //     mem_wb.memoryData
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
    // SW already completed its work in MEM.
    //
    // It does not write anything to the register file.
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

    // --------------------------------------------------------
    // One complete processor clock cycle
    // --------------------------------------------------------
    //
    // Each stage reads the CURRENT pipeline registers and
    // writes its result into the NEXT pipeline registers.
    //
    // Therefore, we must NOT immediately overwrite the
    // current pipeline registers.
    //
    // First:
    //
    //     CURRENT → stages → NEXT
    //
    // Then, at the simulated clock edge:
    //
    //     NEXT → CURRENT
    // --------------------------------------------------------

    // Advance the clock.
    cycle++;


    // ========================================================
    // Phase 1: Execute all five pipeline stages
    // ========================================================
    //
    // We call them from WB → IF.
    //
    // Each stage reads CURRENT state, while writing NEXT state.
    // ========================================================

    // Stage 5
    writeBackStage();

    // Stage 4
    memoryStage();

    // Stage 3
    executeStage();

    // Stage 2
    decodeStage();

    // Stage 1
    fetchStage();


    // ========================================================
    // Phase 2: Simulated clock edge
    // ========================================================
    //
    // All pipeline registers update simultaneously.
    //
    // This is analogous to flip-flops capturing their inputs
    // on a real processor clock edge.
    // ========================================================

    if_id = next_if_id;

    id_ex = next_id_ex;

    ex_mem = next_ex_mem;

    mem_wb = next_mem_wb;
}