#include <iostream>

#include "CPU.h"

int main() {

    CPU cpu;

    // ========================================================
    // Program
    // ========================================================
    //
    // I1: ADD $t0, $t1, $t2
    // I2: SUB $t3, $t0, $t4
    //
    // I2 depends on the result produced by I1.
    // ========================================================

    cpu.instructionMemory.push_back(0x012A4020); // ADD $t0,$t1,$t2
    cpu.instructionMemory.push_back(0x010C5822); // SUB $t3,$t0,$t4


    // --------------------------------------------------------
    // Initial register values
    // --------------------------------------------------------

    cpu.writeRegister(9, 10);   // $t1 = 10
    cpu.writeRegister(10, 20);  // $t2 = 20

    cpu.writeRegister(12, 5);   // $t4 = 5


    // ========================================================
    // Run enough cycles for both instructions to finish.
    // ========================================================

    for (int i = 0; i < 6; i++) {

        cpu.step();

        std::cout << "Cycle "
                  << cpu.cycle
                  << "\n";

        std::cout << "  $t0 = "
                  << cpu.readRegister(8)
                  << "\n";

        std::cout << "  $t3 = "
                  << cpu.readRegister(11)
                  << "\n";

        std::cout << "\n";
    }


    // ========================================================
    // Expected result WITHOUT hazard handling:
    //
    // I1:
    //     $t0 = 10 + 20 = 30
    //
    // I2 should be:
    //     $t3 = 30 - 5 = 25
    //
    // But our current pipeline will likely produce:
    //     $t3 = 0 - 5
    // because I2 reads the old $t0.
    // ========================================================

    std::cout << "Final:\n";
    std::cout << "$t0 = "
              << cpu.readRegister(8)
              << "\n";

    std::cout << "$t3 = "
              << cpu.readRegister(11)
              << "\n";


    return 0;
}