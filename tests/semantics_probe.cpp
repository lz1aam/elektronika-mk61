#include "calculator_core.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {
struct Coordinate { unsigned row; unsigned column; };

void pressAndSettle(pmk::CalculatorCore& core, Coordinate key) {
    core.pressMatrixKey(key.row, key.column);
    core.advanceTicks(50000);
    core.releaseKey();
    core.advanceTicks(500000);
}
}

int main() {
    std::vector<Coordinate> operations;
    operations.push_back({1, 0});
    for (unsigned row = 2; row <= 3; ++row)
        for (unsigned column = 0; column < 10; ++column)
            operations.push_back({row, column});

    pmk::CalculatorCore afterOne;
    afterOne.advanceTicks(250000);
    pressAndSettle(afterOne, {1, 1});

    pmk::CalculatorCore afterTwo;
    afterTwo.advanceTicks(250000);
    pressAndSettle(afterTwo, {1, 2});
    for (const Coordinate candidate : operations) {
        auto core = afterTwo;
        pressAndSettle(core, candidate);
        if (core.displayText().starts_with("-2."))
            std::cout << "sign candidate " << candidate.row << ','
                      << candidate.column << " -> [" << core.displayText()
                      << "]\n";
    }

    pmk::CalculatorCore afterFive;
    afterFive.advanceTicks(250000);
    pressAndSettle(afterFive, {1, 5});
    for (const Coordinate candidate : operations) {
        auto core = afterFive;
        pressAndSettle(core, candidate);
        if (core.displayText().starts_with(" 0."))
            std::cout << "zeroing candidate " << candidate.row << ','
                      << candidate.column << " -> [" << core.displayText()
                      << "]\n";
    }

    for (const Coordinate storeCandidate : operations) {
        auto stored = afterFive;
        pressAndSettle(stored, storeCandidate);
        pressAndSettle(stored, {1, 2});
        pressAndSettle(stored, {2, 8}); // behaviorally identified Cx candidate
        for (const Coordinate recallCandidate : operations) {
            auto core = stored;
            pressAndSettle(core, recallCandidate);
            pressAndSettle(core, {1, 2});
            if (core.displayText().starts_with(" 5."))
                std::cout << "store " << storeCandidate.row << ','
                          << storeCandidate.column << " recall "
                          << recallCandidate.row << ',' << recallCandidate.column
                          << " -> [" << core.displayText() << "]\n";
        }
    }

    for (const Coordinate enterCandidate : operations) {
        auto afterEnterAndTwo = afterOne;
        pressAndSettle(afterEnterAndTwo, enterCandidate);
        pressAndSettle(afterEnterAndTwo, {1, 2});
        for (const Coordinate operationCandidate : operations) {
            auto core = afterEnterAndTwo;
            pressAndSettle(core, operationCandidate);
            const std::string display = core.displayText();
            if (display.starts_with(" 3."))
                std::cout << "enter " << enterCandidate.row << ',' << enterCandidate.column
                          << " operation " << operationCandidate.row << ','
                          << operationCandidate.column << " -> [" << display << "]\n";
        }
    }
}
