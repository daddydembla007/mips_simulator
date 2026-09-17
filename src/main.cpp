#include <iostream>

#include "CPU.h"

int main() {

    CPU cpu;

    // ========================================================
    // Program
    // ========================================================
    //
    // Address 0:
    //     BEQ $t0, $t1, 2
    //
    // Address 4:
    //     ADDI $t2, $zero, 999   <-- WRONG PATH
    //
    // Address 8:
    //     ADDI $t2, $zero, 888   <-- WRONG PATH
    //
    // Address 12:
    //     ADDI $t3, $zero, 42    <-- BRANCH TARGET
    //
    // Since $t0 == $t1, the BEQ is taken.
    //
    // Therefore:
    //
    //     $t2 must remain 0
    //     $t3 must become 42
    // ========================================================


    // --------------------------------------------------------
    // BEQ $t0, $t1, 2
    //
    // opcode = 4
    // rs     = 8  ($t0)
    // rt     = 9  ($t1)
    // immediate = 2
    // --------------------------------------------------------

    uint32_t beq =
        (4u << 26) |
        (8u << 21) |
        (9u << 16) |
        2u;


    // --------------------------------------------------------
    // ADDI $t2, $zero, 999
    //
    // opcode = 8
    // rs     = 0  ($zero)
    // rt     = 10 ($t2)
    // --------------------------------------------------------

    uint32_t wrongInstruction1 =
        (8u << 26) |
        (0u << 21) |
        (10u << 16) |
        999u;


    // --------------------------------------------------------
    // ADDI $t2, $zero, 888
    // --------------------------------------------------------

    uint32_t wrongInstruction2 =
        (8u << 26) |
        (0u << 21) |
        (10u << 16) |
        888u;


    // --------------------------------------------------------
    // ADDI $t3, $zero, 42
    //
    // This is the correct branch target at address 12.
    // --------------------------------------------------------

    uint32_t targetInstruction =
        (8u << 26) |
        (0u << 21) |
        (11u << 16) |
        42u;


    // --------------------------------------------------------
    // Load instructions into instruction memory.
    // --------------------------------------------------------

    cpu.instructionMemory.push_back(beq);
    cpu.instructionMemory.push_back(wrongInstruction1);
    cpu.instructionMemory.push_back(wrongInstruction2);
    cpu.instructionMemory.push_back(targetInstruction);


    // --------------------------------------------------------
    // Make BEQ condition true.
    // --------------------------------------------------------

    cpu.writeRegister(8, 10);   // $t0 = 10
    cpu.writeRegister(9, 10);   // $t1 = 10


    // ========================================================
    // Run pipeline
    // ========================================================

    for (int i = 0; i < 8; i++) {

        cpu.step();

        std::cout << "\n========================================\n";
        std::cout << "Cycle " << cpu.cycle << "\n";
        std::cout << "========================================\n";

        std::cout << "PC = "
                  << cpu.PC
                  << "\n";


        // ----------------------------------------------------
        // IF/ID
        // ----------------------------------------------------

        std::cout << "IF/ID : ";

        if (cpu.if_id.valid) {

            Instruction instruction(
                cpu.if_id.instruction
            );

            DecodedInstruction decoded =
                decodeInstruction(instruction);

            if (decoded.operation == Operation::BEQ)
                std::cout << "BEQ";

            else if (decoded.operation == Operation::ADDI)
                std::cout << "ADDI";

            else
                std::cout << "OTHER";

        } else {

            std::cout << "-";
        }


        // ----------------------------------------------------
        // ID/EX
        // ----------------------------------------------------

        std::cout << "\nID/EX : ";

        if (cpu.id_ex.valid) {

            if (cpu.id_ex.operation == Operation::BEQ)
                std::cout << "BEQ";

            else if (cpu.id_ex.operation == Operation::ADDI)
                std::cout << "ADDI";

            else
                std::cout << "OTHER";

        } else {

            std::cout << "BUBBLE";
        }


        // ----------------------------------------------------
        // EX/MEM
        // ----------------------------------------------------

        std::cout << "\nEX/MEM: ";

        if (cpu.ex_mem.valid) {

            if (cpu.ex_mem.operation == Operation::ADDI)
                std::cout << "ADDI";

            else
                std::cout << "OTHER";

        } else {

            std::cout << "-";
        }


        // ----------------------------------------------------
        // MEM/WB
        // ----------------------------------------------------

        std::cout << "\nMEM/WB: ";

        if (cpu.mem_wb.valid) {

            if (cpu.mem_wb.operation == Operation::ADDI)
                std::cout << "ADDI";

            else
                std::cout << "OTHER";

        } else {

            std::cout << "-";
        }


        // ----------------------------------------------------
        // Important registers
        // ----------------------------------------------------

        std::cout << "\n$t2 = "
                  << cpu.readRegister(10);

        std::cout << "\n$t3 = "
                  << cpu.readRegister(11);

        std::cout << "\n";
    }


    // ========================================================
    // Final results
    // ========================================================

    std::cout << "\nFinal:\n";

    std::cout << "$t2 = "
              << cpu.readRegister(10)
              << "\n";

    std::cout << "$t3 = "
              << cpu.readRegister(11)
              << "\n";


    return 0;
}