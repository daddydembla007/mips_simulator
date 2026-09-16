#ifndef ALU_H
#define ALU_H

#include <cstdint>
#include "Decoder.h"

class ALU {
public:
    static uint32_t execute(
        Operation operation,
        uint32_t operand1,
        uint32_t operand2
    );
};

#endif