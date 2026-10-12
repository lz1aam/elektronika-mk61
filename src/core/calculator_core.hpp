#pragma once

#include "delay_memory.hpp"
#include "microcode_chip.hpp"
#include "calculator_io.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace pmk {

enum class AngleMode { Radians, Degrees, Grads };
enum class CalculatorModel { MK61, MK54 };

class CalculatorCore {
public:
    using Program = std::array<std::uint8_t, 105>;
    using StackDisplaySnapshots = std::array<hardware::VfdSnapshot, 4>;

    CalculatorCore();

    void reset();
    void setAngleMode(AngleMode mode);
    void setModel(CalculatorModel model);
    CalculatorModel model() const { return model_; }

    // EMU145 keyboard matrix: row is 1..3, column is 0..9.
    void pressMatrixKey(unsigned row, unsigned column);
    void releaseKey();
    void advanceTicks(std::size_t ticks);
    void advanceRealtimeSlice(); // 168 microticks, matching upstream's 1 ms loop slice

    const std::array<std::uint8_t, 12>& displaySegments() const {
        return hardware_.display().codes();
    }
    const hardware::VfdSnapshot& displaySnapshot() const {
        return hardware_.display().snapshot();
    }
    // X, Y, Z, T, decoded from the circulating calculator memories at the
    // same synchronization point used by PMK-Android's register dump.
    const StackDisplaySnapshots& stackDisplaySnapshots() const {
        return stackDisplaySnapshots_;
    }
    std::string displayText() const;
    // PMK-Android's digital font uses '/' as a fixed-width blank decimal
    // placeholder. This form keeps all twelve digits aligned in the UI.
    std::string displayGlyphText() const;
    // PMK-Android-compatible physical program-memory ordering. Both calls
    // advance the complete emulated machine to the required IR2 alignment;
    // they intentionally mutate timing state even when only exporting data.
    Program exportProgram();
    void importProgram(const Program& program);
    bool keyContactActive() const {
        return hardware_.keyboard().contactActive();
    }
    std::uint64_t tickCount() const { return ticks_; }

private:
    void tick();
    void alignProgramMemory();
    void refreshStackDisplaySnapshots();
    hardware::VfdSnapshot stackValueSnapshot(unsigned chip,
                                              unsigned address) const;
    std::uint8_t readProgramNibble(unsigned chip, unsigned address) const;
    void writeProgramNibble(unsigned chip, unsigned address, std::uint8_t value);

    MicrocodeChip ik1302_;
    MicrocodeChip ik1303_;
    MicrocodeChip ik1306_;
    DelayMemory ir2_1_;
    DelayMemory ir2_2_;
    hardware::CalculatorIo hardware_;
    StackDisplaySnapshots stackDisplaySnapshots_{};
    bool chain_{};
    bool sync_{};
    unsigned dcycle_{};
    AngleMode mode_{AngleMode::Radians};
    CalculatorModel model_{CalculatorModel::MK61};
    std::uint64_t ticks_{};
    unsigned memorySyncCounter_{};
};

} // namespace pmk

