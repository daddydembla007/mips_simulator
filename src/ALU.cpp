#include "ALU.h"
#include <stdexcept>

uint32_t ALU::execute(
    Operation operation,
    uint32_t operand1,
    uint32_t operand2
) {

    switch (operation) {

        case Operation::ADD:
        case Operation::ADDI:
            return operand1 + operand2;

        case Operation::SUB:
            return operand1 - operand2;

        case Operation::AND:
            return operand1 & operand2;

        case Operation::OR:
            return operand1 | operand2;

        case Operation::SLT:
            return (static_cast<int32_t>(operand1)
                    < static_cast<int32_t>(operand2)) ? 1 : 0;

        default:
            throw std::runtime_error("Unsupported ALU operation");
    }
}