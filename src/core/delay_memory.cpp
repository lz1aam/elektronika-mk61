/*
 * Adapted from EMU145 cmem.cpp by Felix Lazarev, GPL-3.0.
 */
#include "delay_memory.hpp"
#include <stdexcept>
namespace pmk {
bool DelayMemory::tick(bool input) {
    // The oldest bit exits as the incoming bit replaces it. Moving the head
    // models the same serial delay without copying the other 1007 bits.
    const bool output = bits_[head_];
    bits_[head_] = input;
    if (++head_ == BitLength)
        head_ = 0;
    return output;
}

std::uint8_t DelayMemory::nibbleFromHead(unsigned nibbleOffset) const {
    std::uint8_t value = 0;
    if (nibbleOffset >= BitLength / 4)
        throw std::out_of_range("delay-memory nibble offset");
    const unsigned bitOffset = nibbleOffset * 4;
    for (unsigned bit = 0; bit < 4; ++bit)
        if (bits_[(head_ + bitOffset + bit) % BitLength])
            value |= static_cast<std::uint8_t>(1U << bit);
    return value;
}

void DelayMemory::setNibbleFromHead(unsigned nibbleOffset, std::uint8_t value) {
    if (nibbleOffset >= BitLength / 4)
        throw std::out_of_range("delay-memory nibble offset");
    const unsigned bitOffset = nibbleOffset * 4;
    for (unsigned bit = 0; bit < 4; ++bit)
        bits_[(head_ + bitOffset + bit) % BitLength] =
            (value & (1U << bit)) != 0;
}
}

