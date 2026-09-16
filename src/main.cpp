#include <iostream>

#include "CPU.h"
#include "Instruction.h"
#include "Decoder.h"

int main() {

    CPU cpu;

    uint32_t machineCode = 0x012A4020;

    Instruction instruction(machineCode);

    DecodedInstruction decoded =
        decodeInstruction(instruction);

    std::cout << "RS: "
              << static_cast<int>(decoded.rs) << "\n";

    std::cout << "RT: "
              << static_cast<int>(decoded.rt) << "\n";

    std::cout << "RD: "
              << static_cast<int>(decoded.rd) << "\n";

    if (decoded.operation == Operation::ADD) {
        std::cout << "Operation: ADD\n";
    }

    return 0;
}