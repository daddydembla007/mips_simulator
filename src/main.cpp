#include <iostream>

#include "CPU.h"
#include "Instruction.h"
#include "Decoder.h"

int main() {

    CPU cpu;

    /*
        We will use:

        $t1 (R9) = 100
        $t0 (R8) = 123456
    */

    cpu.writeRegister(9, 100);
    cpu.writeRegister(8, 123456);

    /*
        SW:

            sw $t0, 4($t1)

        Effective address:

            100 + 4 = 104

        So 123456 should be stored at memory address 104.
    */

    uint32_t swCode = 0xAD280004;

    Instruction swInstruction(swCode);

    DecodedInstruction decodedSW =
        decodeInstruction(swInstruction);

    cpu.execute(decodedSW);

    /*
        Clear $t0 so that we can prove LW
        actually retrieves the value from memory.
    */
    cpu.writeRegister(8, 0);

    /*
        LW:

            lw $t0, 4($t1)

        This should load 123456 back into $t0.
    */

    uint32_t lwCode = 0x8D280004;

    Instruction lwInstruction(lwCode);

    DecodedInstruction decodedLW =
        decodeInstruction(lwInstruction);

    cpu.execute(decodedLW);

    std::cout << "$t0 = "
              << cpu.readRegister(8)
              << "\n";

    return 0;
}