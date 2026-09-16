#include <iostream>

#include "CPU.h"

int main() {

    CPU cpu;

    // Put two dummy instructions into instruction memory.
    cpu.instructionMemory.push_back(0x012A4020);
    cpu.instructionMemory.push_back(0x014B4820);

    // PC starts at 0.
    std::cout << "Initial PC: " << cpu.PC << "\n";

    // Fetch first instruction
    cpu.fetchStage();

    std::cout << "After first fetch:\n";
    std::cout << "PC = " << cpu.PC << "\n";
    std::cout << "IF/ID valid = " << cpu.if_id.valid << "\n";
    std::cout << "IF/ID PC = " << cpu.if_id.pc << "\n";
    std::cout << "IF/ID instruction = 0x"
              << std::hex << cpu.if_id.instruction
              << std::dec << "\n";

    // Fetch second instruction
    cpu.fetchStage();

    std::cout << "\nAfter second fetch:\n";
    std::cout << "PC = " << cpu.PC << "\n";
    std::cout << "IF/ID PC = " << cpu.if_id.pc << "\n";
    std::cout << "IF/ID instruction = 0x"
              << std::hex << cpu.if_id.instruction
              << std::dec << "\n";

    return 0;
}