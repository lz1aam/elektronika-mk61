#include "calculator_core.hpp"
#include <iostream>

int main() {
    for (unsigned row = 1; row <= 3; ++row) {
        for (unsigned col = 0; col < 10; ++col) {
            pmk::CalculatorCore core;
            core.advanceTicks(250000);
            const auto before = core.displayText();
            core.pressMatrixKey(row, col);
            core.advanceTicks(250000);
            const auto after = core.displayText();
            if (before != after)
                std::cout << row << "," << col << " [" << before << "] -> [" << after << "]\n";
        }
    }
}

