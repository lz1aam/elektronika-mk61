#pragma once

#include <cstdint>

namespace pmk {

struct MatrixKey {
    std::uint8_t row;
    std::uint8_t column;
};

// PMK-Android's activity_main.xml stores this exact two-digit code in each
// button's android:tag. Emulator.keypad() maps its tens digit to the electrical
// keyboard line and its units digit to the display-scan column.
enum class PhysicalKey : std::uint8_t {
    Digit0 = 10, Digit1 = 11, Digit2 = 12, Digit3 = 13, Digit4 = 14,
    Digit5 = 15, Digit6 = 16, Digit7 = 17, Digit8 = 18, Digit9 = 19,
    Add = 20, Subtract = 21, Multiply = 22, Divide = 23, ExchangeXY = 24,
    Decimal = 25, ChangeSign = 26, Exponent = 27, Clear = 28, Enter = 29,
    RunStop = 30, Branch = 31, Return = 32, Subroutine = 33, Store = 34,
    StepForward = 35, Recall = 36, StepBack = 37, K = 38, F = 39
};

constexpr MatrixKey matrixKey(PhysicalKey key) {
    const auto code = static_cast<std::uint8_t>(key);
    return {static_cast<std::uint8_t>(code / 10U),
            static_cast<std::uint8_t>(code % 10U)};
}

} // namespace pmk
