/*
 * Adapted from EMU145 cmem.cpp by Felix Lazarev, GPL-3.0.
 */
#include "delay_memory.hpp"
namespace pmk {
bool DelayMemory::tick(bool input) {
    const bool output = bits_[0];
    for (unsigned i = 0; i + 1 < BitLength; ++i)
        bits_[i] = bits_[i + 1];
    bits_[BitLength - 1] = input;
    return output;
}

std::uint8_t DelayMemory::nibbleFromHead(unsigned nibbleOffset) const {
    std::uint8_t value = 0;
    const unsigned bitOffset = nibbleOffset * 4;
    for (unsigned bit = 0; bit < 4; ++bit)
        if (bits_.at(bitOffset + bit))
            value |= static_cast<std::uint8_t>(1U << bit);
    return value;
}

void DelayMemory::setNibbleFromHead(unsigned nibbleOffset, std::uint8_t value) {
    const unsigned bitOffset = nibbleOffset * 4;
    for (unsigned bit = 0; bit < 4; ++bit)
        bits_.at(bitOffset + bit) = (value & (1U << bit)) != 0;
}
}

