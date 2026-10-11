#pragma once

#include "calculator_core.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace pmk {

struct ParsedProgram {
    CalculatorCore::Program bytes{};
    std::size_t size{};
};

// Canonical PMK-Android address/mnemonic text representation.
std::string exportProgramText(const CalculatorCore::Program& program);
ParsedProgram importProgramText(std::string_view text);

} // namespace pmk
