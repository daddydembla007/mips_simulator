#include <iostream>
#include <iomanip>
#include <string>

#include "CPU.h"
#include "ProgramLoader.h"
#include "Decoder.h"


// ============================================================
// Convert Operation enum into readable text.
// ============================================================

std::string operationName(Operation operation) {

    switch (operation) {

        case Operation::NOP:
            return "NOP";

        case Operation::ADD:
            return "ADD";

        case Operation::SUB:
            return "SUB";

        case Operation::AND:
            return "AND";

        case Operation::OR:
            return "OR";

        case Operation::SLT:
            return "SLT";

        case Operation::ADDI:
            return "ADDI";

        case Operation::LW:
            return "LW";

        case Operation::SW:
            return "SW";

        case Operation::BEQ:
            return "BEQ";

        case Operation::BNE:
            return "BNE";

        case Operation::J:
            return "J";

        case Operation::INVALID:
            return "INVALID";

        default:
            return "UNKNOWN";
    }
}


// ============================================================
// IF/ID operation
// ============================================================

std::string getIFIDOperation(const CPU& cpu) {

    if (!cpu.if_id.valid) {
        return "-";
    }

    Instruction instruction(cpu.if_id.instruction);

    DecodedInstruction decoded =
        decodeInstruction(instruction);

    return operationName(decoded.operation);
}


// ============================================================
// ID/EX operation
// ============================================================

std::string getIDEXOperation(const CPU& cpu) {

    if (!cpu.id_ex.valid) {
        return "BUBBLE";
    }

    return operationName(cpu.id_ex.operation);
}


// ============================================================
// EX/MEM operation
// ============================================================

std::string getEXMEMOperation(const CPU& cpu) {

    if (!cpu.ex_mem.valid) {
        return "-";
    }

    return operationName(cpu.ex_mem.operation);
}


// ============================================================
// MEM/WB operation
// ============================================================

std::string getMEMWBOperation(const CPU& cpu) {

    if (!cpu.mem_wb.valid) {
        return "-";
    }

    return operationName(cpu.mem_wb.operation);
}


// ============================================================
// Initialize registers/memory for different tests.
// ============================================================

void initializeTestState(
    CPU& cpu,
    const std::string& filename
) {

    // --------------------------------------------------------
    // Default values
    // --------------------------------------------------------

    cpu.writeRegister(5, 12);     // $a1
    cpu.writeRegister(6, 5);      // $a2
    cpu.writeRegister(7, 20);     // $a3

    cpu.writeRegister(9, 10);     // $t1
    cpu.writeRegister(10, 5);     // $t2
    cpu.writeRegister(11, 20);    // $t3

    cpu.writeRegister(15, 12);    // $t7
    cpu.writeRegister(24, 10);    // $t8


    // --------------------------------------------------------
    // Arithmetic
    // --------------------------------------------------------

    if (filename.find("arithmetic") != std::string::npos) {

        cpu.writeRegister(5, 12);
        cpu.writeRegister(6, 5);
        cpu.writeRegister(7, 20);
    }


    // --------------------------------------------------------
    // Memory
    // --------------------------------------------------------

    else if (filename.find("memory") != std::string::npos) {

        cpu.writeRegister(9, 100);
        cpu.writeRegister(10, 55);

        cpu.writeMemoryWord(100, 30);
    }


    // --------------------------------------------------------
    // Forwarding
    // --------------------------------------------------------

    else if (filename.find("forwarding") != std::string::npos) {

        cpu.writeRegister(9, 10);
        cpu.writeRegister(10, 5);
        cpu.writeRegister(11, 20);
    }


    // --------------------------------------------------------
    // Load-use hazard
    // --------------------------------------------------------

    else if (filename.find("load_use") != std::string::npos) {

        cpu.writeRegister(9, 10);
        cpu.writeRegister(10, 5);

        cpu.writeMemoryWord(10, 30);
    }


    // --------------------------------------------------------
    // Branch taken
    // --------------------------------------------------------

    else if (filename.find("branch_taken")
             != std::string::npos) {

        cpu.writeRegister(9, 10);
        cpu.writeRegister(10, 10);
    }


    // --------------------------------------------------------
    // Branch not taken
    // --------------------------------------------------------

    else if (filename.find("branch_not_taken")
             != std::string::npos) {

        cpu.writeRegister(9, 10);
        cpu.writeRegister(10, 20);
    }


    // --------------------------------------------------------
    // BNE
    // --------------------------------------------------------

    else if (filename.find("bne") != std::string::npos) {

        cpu.writeRegister(9, 10);
        cpu.writeRegister(10, 20);
    }


    // --------------------------------------------------------
    // Jump
    // --------------------------------------------------------

    else if (filename.find("jump") != std::string::npos) {

        // No special initialization needed.
    }
}


// ============================================================
// MAIN
// ============================================================

int main(int argc, char* argv[]) {

    // --------------------------------------------------------
    // Command-line argument check.
    // --------------------------------------------------------

    if (argc != 2) {

        std::cerr
            << "Usage: "
            << argv[0]
            << " <program_file>\n";

        return 1;
    }


    // --------------------------------------------------------
    // Program filename.
    // --------------------------------------------------------

    std::string filename = argv[1];


    // --------------------------------------------------------
    // Create CPU.
    // --------------------------------------------------------

    CPU cpu;


    // --------------------------------------------------------
    // Load program.
    // --------------------------------------------------------

    try {

        cpu.instructionMemory =
            ProgramLoader::loadProgram(filename);

    }
    catch (const std::exception& e) {

        std::cerr
            << "Error loading program: "
            << e.what()
            << std::endl;

        return 1;
    }


    // --------------------------------------------------------
    // Initialize machine state.
    // --------------------------------------------------------

    initializeTestState(cpu, filename);


    // ========================================================
    // Program information
    // ========================================================

    std::cout
        << "\n============================================================\n";

    std::cout
        << "              MIPS32 PIPELINE SIMULATOR\n";

    std::cout
        << "============================================================\n";

    std::cout
        << "Program      : "
        << filename
        << "\n";

    std::cout
        << "Instructions : "
        << cpu.instructionMemory.size()
        << "\n";

    std::cout
        << "============================================================\n";


    // --------------------------------------------------------
    // Number of cycles.
    //
    // +6 gives enough time for:
    //
    //     pipeline fill
    //     normal execution
    //     stalls
    //     control flushes
    //     pipeline drain
    // --------------------------------------------------------

    const int totalCycles =
        static_cast<int>(cpu.instructionMemory.size()) + 6;


    // ========================================================
    // Pipeline simulation
    // ========================================================

    for (int i = 0; i < totalCycles; i++) {

        cpu.step();


        // ----------------------------------------------------
        // Cycle header
        // ----------------------------------------------------

        std::cout
            << "\n------------------------------------------------------------\n";

        std::cout
            << "Cycle "
            << cpu.cycle
            << "    PC = "
            << cpu.PC
            << "\n";

        std::cout
            << "------------------------------------------------------------\n";


        // ----------------------------------------------------
        // Pipeline table
        // ----------------------------------------------------

        std::cout
            << std::left
            << std::setw(12)
            << "Stage"
            << std::setw(15)
            << "Instruction"
            << "\n";

        std::cout
            << "------------------------------------------------------------\n";

        std::cout
            << std::left
            << std::setw(12)
            << "IF/ID"
            << std::setw(15)
            << getIFIDOperation(cpu)
            << "\n";

        std::cout
            << std::left
            << std::setw(12)
            << "ID/EX"
            << std::setw(15)
            << getIDEXOperation(cpu)
            << "\n";

        std::cout
            << std::left
            << std::setw(12)
            << "EX/MEM"
            << std::setw(15)
            << getEXMEMOperation(cpu)
            << "\n";

        std::cout
            << std::left
            << std::setw(12)
            << "MEM/WB"
            << std::setw(15)
            << getMEMWBOperation(cpu)
            << "\n";


        // ----------------------------------------------------
        // Pipeline event
        // ----------------------------------------------------

        std::cout
            << "------------------------------------------------------------\n";

        std::cout
            << "Event: "
            << cpu.lastEvent
            << "\n";


        // ----------------------------------------------------
        // Register state
        // ----------------------------------------------------

        std::cout
            << "------------------------------------------------------------\n";

        std::cout
            << "$t0 = "
            << cpu.readRegister(8)
            << "    ";

        std::cout
            << "$t1 = "
            << cpu.readRegister(9)
            << "    ";

        std::cout
            << "$t2 = "
            << cpu.readRegister(10)
            << "    ";

        std::cout
            << "$t3 = "
            << cpu.readRegister(11)
            << "    ";

        std::cout
            << "$t4 = "
            << cpu.readRegister(12)
            << "\n";
    }


    // ========================================================
    // FINAL PERFORMANCE STATISTICS
    // ========================================================

    std::cout
        << "\n\n============================================================\n";

    std::cout
        << "                  PERFORMANCE STATISTICS\n";

    std::cout
        << "============================================================\n";


    // --------------------------------------------------------
    // Total cycles
    // --------------------------------------------------------

    std::cout
        << std::left
        << std::setw(30)
        << "Total cycles"
        << cpu.cycle
        << "\n";


    // --------------------------------------------------------
    // Instructions retired
    // --------------------------------------------------------

    std::cout
        << std::left
        << std::setw(30)
        << "Instructions retired"
        << cpu.instructionsRetired
        << "\n";


    // --------------------------------------------------------
    // Stalls
    // --------------------------------------------------------

    std::cout
        << std::left
        << std::setw(30)
        << "Load-use stalls"
        << cpu.stallCount
        << "\n";


    // --------------------------------------------------------
    // Control hazards
    // --------------------------------------------------------

    std::cout
        << std::left
        << std::setw(30)
        << "Control-flow flushes"
        << cpu.flushCount
        << "\n";


    // --------------------------------------------------------
    // Flushed instructions
    // --------------------------------------------------------

    std::cout
        << std::left
        << std::setw(30)
        << "Instructions flushed"
        << cpu.flushedInstructionCount
        << "\n";


    // --------------------------------------------------------
    // CPI
    //
    // CPI = total cycles / instructions retired
    // --------------------------------------------------------

    double cpi = 0.0;

    if (cpu.instructionsRetired > 0) {

        cpi =
            static_cast<double>(cpu.cycle) /
            static_cast<double>(cpu.instructionsRetired);
    }

    std::cout
        << std::left
        << std::setw(30)
        << "CPI"
        << std::fixed
        << std::setprecision(2)
        << cpi
        << "\n";


    // --------------------------------------------------------
    // Final registers
    // --------------------------------------------------------

    std::cout
        << "\n------------------------------------------------------------\n";

    std::cout
        << "Final Register State\n";

    std::cout
        << "------------------------------------------------------------\n";

    std::cout
        << "$t0 = "
        << cpu.readRegister(8)
        << "\n";

    std::cout
        << "$t1 = "
        << cpu.readRegister(9)
        << "\n";

    std::cout
        << "$t2 = "
        << cpu.readRegister(10)
        << "\n";

    std::cout
        << "$t3 = "
        << cpu.readRegister(11)
        << "\n";

    std::cout
        << "$t4 = "
        << cpu.readRegister(12)
        << "\n";


    std::cout
        << "============================================================\n";


    return 0;
}