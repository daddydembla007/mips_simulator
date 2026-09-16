#include "Decoder.h"

DecodedInstruction decodeInstruction(const Instruction& instruction) {

    DecodedInstruction decoded{};

    decoded.operation = Operation::INVALID;

    decoded.rs = instruction.rs;
    decoded.rt = instruction.rt;
    decoded.rd = instruction.rd;

    decoded.immediate = instruction.immediate;
    decoded.address = instruction.address;

    decoded.usesImmediate = false;

    // R-type instructions
    if (instruction.opcode == 0) {

        switch (instruction.funct) {

            case 32:
                decoded.operation = Operation::ADD;
                break;

            case 34:
                decoded.operation = Operation::SUB;
                break;

            case 36:
                decoded.operation = Operation::AND;
                break;

            case 37:
                decoded.operation = Operation::OR;
                break;

            case 42:
                decoded.operation = Operation::SLT;
                break;

            default:
                decoded.operation = Operation::INVALID;
        }
    }

    // ADDI
    else if (instruction.opcode == 8) {
        decoded.operation = Operation::ADDI;
        decoded.usesImmediate = true;
    }

    // LW
    else if (instruction.opcode == 35) {
        decoded.operation = Operation::LW;
        decoded.usesImmediate = true;
    }

    // SW
    else if (instruction.opcode == 43) {
        decoded.operation = Operation::SW;
        decoded.usesImmediate = true;
    }

    // BEQ
    else if (instruction.opcode == 4) {
        decoded.operation = Operation::BEQ;
        decoded.usesImmediate = true;
    }

    // BNE
    else if (instruction.opcode == 5) {
        decoded.operation = Operation::BNE;
        decoded.usesImmediate = true;
    }

    // J
    else if (instruction.opcode == 2) {
        decoded.operation = Operation::J;
    }

    return decoded;
}