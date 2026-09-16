#include <iostream>

#include "CPU.h"
#include "Instruction.h"
#include "Decoder.h"
#include "ALU.h"

int main() {

    CPU cpu;

    uint32_t result;

    result = ALU::execute(
        Operation::ADD,
        10,
        5
    );

    std::cout << "10 + 5 = " << result << "\n";


    result = ALU::execute(
        Operation::SUB,
        10,
        5
    );

    std::cout << "10 - 5 = " << result << "\n";


    result = ALU::execute(
        Operation::AND,
        12,
        10
    );

    std::cout << "12 & 10 = " << result << "\n";


    result = ALU::execute(
        Operation::OR,
        12,
        10
    );

    std::cout << "12 | 10 = " << result << "\n";


    result = ALU::execute(
        Operation::SLT,
        5,
        10
    );

    std::cout << "5 < 10 = " << result << "\n";

    return 0;
}