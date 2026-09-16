#include "CPU.h"

CPU::CPU() {
    reset();
}

void CPU::reset() {
    PC = 0;

    for (int i = 0; i < 32; i++) {
        registers[i] = 0; // all registers initialized to 0 
    }

    instructionMemory.clear();

    dataMemory.resize(1024, 0); // initially 1KB of memory ( can be changed later if needed )
}