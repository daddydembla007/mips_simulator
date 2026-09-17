#include <iostream>

#include "CPU.h"

int main() {

    CPU cpu;

    // --------------------------------------------------------
    // Program:
    //
    //     ADD $t0, $t1, $t2
    //
    // $t0 = R8
    // $t1 = R9
    // $t2 = R10
    //
    // Expected:
    //
    //     $t0 = 10 + 20 = 30
    // --------------------------------------------------------

    cpu.instructionMemory.push_back(0x012A4020);

    // Initialize source registers.
    cpu.writeRegister(9, 10);
    cpu.writeRegister(10, 20);


    // ========================================================
    // Cycle 1: IF
    // ========================================================

    cpu.fetchStage();


    // ========================================================
    // Cycle 2: ID
    // ========================================================

    cpu.decodeStage();


    // ========================================================
    // Cycle 3: EX
    // ========================================================

    cpu.executeStage();


    // ========================================================
    // Cycle 4: MEM
    // ========================================================

    cpu.memoryStage();


    // ========================================================
    // Cycle 5: WB
    // ========================================================

    cpu.writeBackStage();


    // --------------------------------------------------------
    // Check final result
    // --------------------------------------------------------

    std::cout << "Final register values:\n";

    std::cout << "$t1 = "
              << cpu.readRegister(9)
              << "\n";

    std::cout << "$t2 = "
              << cpu.readRegister(10)
              << "\n";

    std::cout << "$t0 = "
              << cpu.readRegister(8)
              << "\n";


    if (cpu.readRegister(8) == 30) {

        std::cout << "\nSUCCESS: ADD completed correctly!\n";

    } else {

        std::cout << "\nERROR: ADD result is incorrect.\n";
    }

    return 0;
}