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
    branchTaken = false;
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
}

void CPU::executeStage() {

    // --------------------------------------------------------
    // Execute (EX) stage
    // --------------------------------------------------------
    //
    // CURRENT:
    //
    //     ID/EX
    //       |
    //       v
    //      EX
    //       |
    //       v
    // NEXT:
    //
    //     next EX/MEM
    //
    // The EX stage:
    //
    // 1. Handles branches
    // 2. Selects ALU operands
    // 3. Performs forwarding when necessary
    // 4. Executes the ALU operation
    // 5. Passes the result to next EX/MEM
    // --------------------------------------------------------


    // --------------------------------------------------------
    // By default, assume no branch is taken this cycle.
    // --------------------------------------------------------

    branchTaken = false;


    // --------------------------------------------------------
    // If ID/EX is empty, there is nothing to execute.
    // --------------------------------------------------------

    if (!id_ex.valid) {

        next_ex_mem.valid = false;

        return;
    }


    // ========================================================
    // NOP
    // ========================================================
    //
    // NOP occupies a pipeline slot but performs no operation.
    //
    // Therefore, it simply becomes a bubble after EX.
    // ========================================================

    if (id_ex.operation == Operation::NOP) {

        next_ex_mem.valid = false;

        return;
    }


    // ========================================================
    // Branch instructions
    // ========================================================
    //
    // BEQ:
    //
    //     branch if rs == rt
    //
    // BNE:
    //
    //     branch if rs != rt
    //
    // Branches don't need the normal ALU → MEM → WB path.
    // ========================================================

    if (id_ex.operation == Operation::BEQ ||
        id_ex.operation == Operation::BNE) {


        // ----------------------------------------------------
        // Values to compare
        // ----------------------------------------------------

        uint32_t value1 =
            id_ex.readData1;

        uint32_t value2 =
            id_ex.readData2;


        // ----------------------------------------------------
        // Forward branch operands if necessary.
        //
        // This is important if a branch depends on a recently
        // produced arithmetic result.
        // ----------------------------------------------------

        // Forward first operand from EX/MEM.
        if (ex_mem.valid &&
            ex_mem.destination != 0 &&
            ex_mem.destination == id_ex.rs &&
            ex_mem.operation != Operation::SW &&
            ex_mem.operation != Operation::LW) {

            value1 =
                ex_mem.aluResult;
        }

        // Otherwise forward from MEM/WB.
        else if (mem_wb.valid &&
                 mem_wb.destination != 0 &&
                 mem_wb.destination == id_ex.rs) {

            if (mem_wb.operation == Operation::LW) {

                value1 =
                    mem_wb.memoryData;

            } else {

                value1 =
                    mem_wb.aluResult;
            }
        }


        // Forward second operand from EX/MEM.
        if (ex_mem.valid &&
            ex_mem.destination != 0 &&
            ex_mem.destination == id_ex.rt &&
            ex_mem.operation != Operation::SW &&
            ex_mem.operation != Operation::LW) {

            value2 =
                ex_mem.aluResult;
        }

        // Otherwise forward from MEM/WB.
        else if (mem_wb.valid &&
                 mem_wb.destination != 0 &&
                 mem_wb.destination == id_ex.rt) {

            if (mem_wb.operation == Operation::LW) {

                value2 =
                    mem_wb.memoryData;

            } else {

                value2 =
                    mem_wb.aluResult;
            }
        }


        // ----------------------------------------------------
        // Determine whether branch is taken.
        // ----------------------------------------------------

        bool taken;

        if (id_ex.operation == Operation::BEQ) {

            // BEQ → branch if equal
            taken =
                (value1 == value2);

        } else {

            // BNE → branch if not equal
            taken =
                (value1 != value2);
        }


        // ----------------------------------------------------
        // Redirect PC if branch is taken.
        // ----------------------------------------------------

        if (taken) {

            branchTaken = true;

            PC =
                id_ex.branchTarget;
        }


        // ----------------------------------------------------
        // Branch does not continue to MEM/WB.
        // ----------------------------------------------------

        next_ex_mem.valid = false;

        return;
    }


    // ========================================================
    // Normal ALU instructions
    // ========================================================


    // --------------------------------------------------------
    // Operand 1
    // --------------------------------------------------------
    //
    // Normally:
    //
    //     operand1 = value of rs
    //
    // But if a previous instruction has already calculated
    // a newer value for rs, use forwarding.
    // --------------------------------------------------------

    uint32_t operand1 =
        id_ex.readData1;


    // --------------------------------------------------------
    // Forward operand 1 from EX/MEM.
    // --------------------------------------------------------

    if (ex_mem.valid &&
        ex_mem.destination != 0 &&
        ex_mem.destination == id_ex.rs &&
        ex_mem.operation != Operation::SW &&
        ex_mem.operation != Operation::LW) {

        operand1 =
            ex_mem.aluResult;
    }


    // --------------------------------------------------------
    // If EX/MEM doesn't have the value, check MEM/WB.
    // --------------------------------------------------------

    else if (mem_wb.valid &&
             mem_wb.destination != 0 &&
             mem_wb.destination == id_ex.rs) {

        if (mem_wb.operation == Operation::LW) {

            operand1 =
                mem_wb.memoryData;

        } else {

            operand1 =
                mem_wb.aluResult;
        }
    }


    // --------------------------------------------------------
    // Operand 2
    // --------------------------------------------------------

    uint32_t operand2;


    // --------------------------------------------------------
    // Immediate instructions
    // --------------------------------------------------------
    //
    // ADDI:
    //
    //     rs + immediate
    //
    // LW/SW:
    //
    //     base + offset
    // --------------------------------------------------------

    if (id_ex.operation == Operation::ADDI ||
        id_ex.operation == Operation::LW ||
        id_ex.operation == Operation::SW) {

        operand2 =
            static_cast<uint32_t>(
                id_ex.immediate
            );
    }


    // --------------------------------------------------------
    // R-type instructions
    // --------------------------------------------------------
    //
    // For R-type instructions:
    //
    //     operand2 = value of rt
    //
    // This value may also need forwarding.
    // --------------------------------------------------------

    else {

        operand2 =
            id_ex.readData2;


        // ----------------------------------------------------
        // Forward operand 2 from EX/MEM.
        // ----------------------------------------------------

        if (ex_mem.valid &&
            ex_mem.destination != 0 &&
            ex_mem.destination == id_ex.rt &&
            ex_mem.operation != Operation::SW &&
            ex_mem.operation != Operation::LW) {

            operand2 =
                ex_mem.aluResult;
        }


        // ----------------------------------------------------
        // Otherwise forward from MEM/WB.
        // ----------------------------------------------------

        else if (mem_wb.valid &&
                 mem_wb.destination != 0 &&
                 mem_wb.destination == id_ex.rt) {

            if (mem_wb.operation == Operation::LW) {

                operand2 =
                    mem_wb.memoryData;

            } else {

                operand2 =
                    mem_wb.aluResult;
            }
        }
    }


    // ========================================================
    // Determine actual ALU operation
    // ========================================================
    //
    // LW and SW aren't themselves ALU operations.
    //
    // Their EX-stage operation is:
    //
    //     base address + offset
    //
    // Therefore:
    //
    //     LW → ALU ADD
    //     SW → ALU ADD
    // ========================================================

    Operation aluOperation =
        id_ex.operation;


    if (id_ex.operation == Operation::LW ||
        id_ex.operation == Operation::SW) {

        aluOperation =
            Operation::ADD;
    }


    // ========================================================
    // Execute ALU
    // ========================================================

    uint32_t result =
        ALU::execute(
            aluOperation,
            operand1,
            operand2
        );


    // ========================================================
    // Write result into NEXT EX/MEM
    // ========================================================

    next_ex_mem.valid = true;


    // ALU result:
    //
    // Arithmetic instruction → actual result
    //
    // LW/SW → calculated memory address
    next_ex_mem.aluResult =
        result;


    // --------------------------------------------------------
    // Store data
    // --------------------------------------------------------
    //
    // Needed for SW.
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
    // SW doesn't write a register.
    // --------------------------------------------------------

    if (id_ex.operation == Operation::ADDI ||
        id_ex.operation == Operation::LW) {

        next_ex_mem.destination =
            id_ex.rt;

    } else {

        next_ex_mem.destination =
            id_ex.rd;
    }


    // --------------------------------------------------------
    // Pass operation to the next pipeline stage.
    // --------------------------------------------------------

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
    // One processor clock cycle
    // --------------------------------------------------------

    cycle++;


    // ========================================================
    // Execute stages from WB → IF
    // ========================================================
    //
    // Every stage reads CURRENT pipeline registers and writes
    // into NEXT pipeline registers.
    // ========================================================

    writeBackStage();

    memoryStage();

    executeStage();


    // ========================================================
    // Control hazard
    // ========================================================
    //
    // executeStage() may have discovered that a branch is
    // taken.
    //
    // If so:
    //
    //     1. PC has already been redirected to branchTarget.
    //     2. The instruction in IF/ID is wrong-path.
    //     3. The instruction currently entering ID/EX is
    //        also wrong-path.
    //
    // Therefore, flush both pipeline registers.
    // ========================================================

    if (branchTaken) {

        // ----------------------------------------------------
        // Kill the instruction currently waiting in IF/ID.
        // ----------------------------------------------------

        next_if_id.valid = false;


        // ----------------------------------------------------
        // Kill the instruction that would have entered ID/EX.
        // ----------------------------------------------------

        next_id_ex.valid = false;


        // ----------------------------------------------------
        // IMPORTANT:
        //
        // Do NOT call decodeStage().
        // Do NOT call fetchStage().
        //
        // The branch has already redirected PC.
        // The correct-path instruction will be fetched during
        // the next cycle.
        // ----------------------------------------------------
    }

    else {

        // ====================================================
        // No branch taken.
        //
        // Normal ID and IF operation.
        // ====================================================

        // Check for load-use hazard.
        bool loadUseHazard =
            hasLoadUseHazard();


        if (loadUseHazard) {

            // ------------------------------------------------
            // Insert a bubble into ID/EX.
            // ------------------------------------------------

            next_id_ex.valid = false;


            // ------------------------------------------------
            // Keep the dependent instruction in IF/ID.
            // ------------------------------------------------

            next_if_id = if_id;


            // ------------------------------------------------
            // Do not fetch a new instruction.
            //
            // PC therefore remains unchanged.
            // ------------------------------------------------
        }

        else {

            // Normal operation.
            decodeStage();

            fetchStage();
        }
    }


    // ========================================================
    // Clock edge
    // ========================================================
    //
    // All pipeline registers update simultaneously.
    // ========================================================

    if_id = next_if_id;

    id_ex = next_id_ex;

    ex_mem = next_ex_mem;

    mem_wb = next_mem_wb;
}

bool CPU::hasLoadUseHazard() const {

    // --------------------------------------------------------
    // Load-use hazard
    // --------------------------------------------------------
    //
    // Example:
    //
    //     LW  $t0, 0($t1)
    //     ADD $t2, $t0, $t3
    //
    // The LW is currently in ID/EX.
    //
    // The ADD is currently in IF/ID.
    //
    // ADD needs $t0, but LW has not produced the loaded
    // value yet.
    // --------------------------------------------------------

    // If there is no instruction in ID/EX,
    // there cannot be a load-use hazard.
    if (!id_ex.valid) {
        return false;
    }

    // Only LW creates the specific load-use hazard we're
    // detecting here.
    if (id_ex.operation != Operation::LW) {
        return false;
    }


    // --------------------------------------------------------
    // Decode the instruction currently waiting in IF/ID.
    // --------------------------------------------------------

    if (!if_id.valid) {
        return false;
    }

    Instruction instruction(if_id.instruction);

    DecodedInstruction decoded =
        decodeInstruction(instruction);


    // The register that LW will eventually write.
    uint8_t loadDestination =
        id_ex.rt;


    // --------------------------------------------------------
    // Does the instruction in ID need that register?
    // --------------------------------------------------------
    //
    // For our currently supported instructions:
    //
    // ADD/SUB/AND/OR/SLT:
    //     use rs and rt
    //
    // ADDI:
    //     uses rs
    //
    // LW:
    //     uses rs
    //
    // SW:
    //     uses rs and rt
    // --------------------------------------------------------

    bool usesRs =
        true;

    bool usesRt =
        false;

    switch (decoded.operation) {

        case Operation::ADD:
        case Operation::SUB:
        case Operation::AND:
        case Operation::OR:
        case Operation::SLT:

            usesRs = true;
            usesRt = true;
            break;

        case Operation::SW:

            usesRs = true;
            usesRt = true;
            break;

        case Operation::ADDI:
        case Operation::LW:

            usesRs = true;
            usesRt = false;
            break;

        default:

            usesRs = false;
            usesRt = false;
            break;
    }


    // --------------------------------------------------------
    // Check for the actual dependency.
    // --------------------------------------------------------

    if (usesRs &&
        decoded.rs == loadDestination) {

        return true;
    }

    if (usesRt &&
        decoded.rt == loadDestination) {

        return true;
    }


    return false;
}