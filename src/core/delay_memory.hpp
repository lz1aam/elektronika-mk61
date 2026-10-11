#pragma once
#include <array>
#include <cstdint>
namespace pmk {
class DelayMemory {
public:
    static constexpr unsigned BitLength = 1008;
    void reset() { bits_.fill(false); }
    bool pretick() const { return bits_[0]; }
    bool tick(bool input);
    std::uint8_t nibbleFromHead(unsigned nibbleOffset) const;
    void setNibbleFromHead(unsigned nibbleOffset, std::uint8_t value);
private:
    std::array<bool, BitLength> bits_{};
};
}

