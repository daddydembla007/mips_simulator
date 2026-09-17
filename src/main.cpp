#include <iostream>

#include "CPU.h"

int main() {

    CPU cpu;

    // ========================================================
    // Program
    // ========================================================
    //
    // I1: LW  $t0, 0($t1)
    // I2: ADD $t3, $t0, $t2
    //
    // I2 immediately needs the value loaded by I1.
    // Therefore, this creates a load-use hazard.
    // ========================================================

    // LW $t0, 0($t1)
    uint32_t lwInstruction =
        (35u << 26) |
        (9u << 21) |
        (8u << 16) |
        0u;

    // ADD $t3, $t0, $t2
    uint32_t addInstruction =
        (0u << 26) |
        (8u << 21) |
        (10u << 16) |
        (11u << 11) |
        32u;

    cpu.instructionMemory.push_back(lwInstruction);
    cpu.instructionMemory.push_back(addInstruction);


    // --------------------------------------------------------
    // Initial register values
    // --------------------------------------------------------

    // $t1 = base address
    cpu.writeRegister(9, 100);

    // $t2 = 5
    cpu.writeRegister(10, 5);

    // Store 30 at memory address 100.
    cpu.writeMemoryWord(100, 30);


    // ========================================================
    // Run the pipeline
    // ========================================================

    for (int i = 0; i < 7; i++) {

        cpu.step();

        std::cout << "\n========================================\n";
        std::cout << "Cycle " << cpu.cycle << "\n";
        std::cout << "========================================\n";


        std::cout << "IF/ID : ";

        if (cpu.if_id.valid) {

            Instruction instruction(
                cpu.if_id.instruction
            );

            DecodedInstruction decoded =
                decodeInstruction(instruction);

            if (decoded.operation == Operation::LW)
                std::cout << "LW";

            else if (decoded.operation == Operation::ADD)
                std::cout << "ADD";

            else
                std::cout << "OTHER";

        } else {

            std::cout << "-";
        }


        std::cout << "\nID/EX : ";

        if (cpu.id_ex.valid) {

            if (cpu.id_ex.operation == Operation::LW)
                std::cout << "LW";

            else if (cpu.id_ex.operation == Operation::ADD)
                std::cout << "ADD";

            else
                std::cout << "OTHER";

        } else {

            std::cout << "BUBBLE";
        }


        std::cout << "\nEX/MEM: ";

        if (cpu.ex_mem.valid) {

            if (cpu.ex_mem.operation == Operation::LW)
                std::cout << "LW";

            else if (cpu.ex_mem.operation == Operation::ADD)
                std::cout << "ADD";

            else
                std::cout << "OTHER";

        } else {

            std::cout << "-";
        }


        std::cout << "\nMEM/WB: ";

        if (cpu.mem_wb.valid) {

            if (cpu.mem_wb.operation == Operation::LW)
                std::cout << "LW";

            else if (cpu.mem_wb.operation == Operation::ADD)
                std::cout << "ADD";

            else
                std::cout << "OTHER";

        } else {

            std::cout << "-";
        }


        // Show the important registers.
        std::cout << "\n$t0 = "
                  << cpu.readRegister(8);

        std::cout << "\n$t3 = "
                  << cpu.readRegister(11);

        std::cout << "\n";
    }


    // ========================================================
    // Final result
    // ========================================================

    std::cout << "\nFinal:\n";

    std::cout << "$t0 = "
              << cpu.readRegister(8)
              << "\n";

    std::cout << "$t3 = "
              << cpu.readRegister(11)
              << "\n";


    return 0;
}