/*
 * Adapted from EMU145 by Felix Lazarev.
 * Emulator code GPL-3.0; ROM content copyright (c) 198x Trokhimenko Y.K. and others.
 */
#pragma once

#include <cstdint>

namespace pmk {
constexpr unsigned MCU_BITLEN = 168;

// Numeric ROM bit positions, independent of byte order and compiler bitfield
// layout. Accessors decode the stored word without union type-punning.
struct MicroInstruction {
    std::uint32_t raw{};

    constexpr std::uint32_t a_r() const { return (raw >> 0) & 1U; }
    constexpr std::uint32_t a_m() const { return (raw >> 1) & 1U; }
    constexpr std::uint32_t a_st() const { return (raw >> 2) & 1U; }
    constexpr std::uint32_t a_nr() const { return (raw >> 3) & 1U; }
    constexpr std::uint32_t a_10nl() const { return (raw >> 4) & 1U; }
    constexpr std::uint32_t a_s() const { return (raw >> 5) & 1U; }
    constexpr std::uint32_t a_4() const { return (raw >> 6) & 1U; }
    constexpr std::uint32_t b_s() const { return (raw >> 7) & 1U; }
    constexpr std::uint32_t b_ns() const { return (raw >> 8) & 1U; }
    constexpr std::uint32_t b_s1() const { return (raw >> 9) & 1U; }
    constexpr std::uint32_t b_6() const { return (raw >> 10) & 1U; }
    constexpr std::uint32_t b_1() const { return (raw >> 11) & 1U; }
    constexpr std::uint32_t g_l() const { return (raw >> 12) & 1U; }
    constexpr std::uint32_t g_nl() const { return (raw >> 13) & 1U; }
    constexpr std::uint32_t g_nt() const { return (raw >> 14) & 1U; }
    constexpr std::uint32_t r0() const { return (raw >> 15) & 7U; }
    constexpr std::uint32_t r_1() const { return (raw >> 18) & 1U; }
    constexpr std::uint32_t r_2() const { return (raw >> 19) & 1U; }
    constexpr std::uint32_t m() const { return (raw >> 20) & 1U; }
    constexpr std::uint32_t l() const { return (raw >> 21) & 1U; }
    constexpr std::uint32_t s() const { return (raw >> 22) & 3U; }
    constexpr std::uint32_t s1() const { return (raw >> 24) & 3U; }
    constexpr std::uint32_t st() const { return (raw >> 26) & 3U; }
};

class MicrocodeChip {
public:
    MicrocodeChip() = default;
    void init();
    void pretick(bool rin);
    bool tick(bool rin, bool k1, bool k2, unsigned* dcycle,
              bool* syncout, unsigned char* segment);
    bool strobe();
    std::uint8_t memoryNibble(unsigned index) const;
    void setMemoryNibble(unsigned index, std::uint8_t value);

    MicroInstruction ucrom[68]{};
    unsigned char asprom[128][9]{};
    std::uint32_t cmdrom[256]{};

    bool rm[MCU_BITLEN]{};
    bool rr[MCU_BITLEN]{};
    bool rst[MCU_BITLEN]{};
    bool rs[4]{};
    bool rs1[4]{};
    bool rh[4]{};
    bool dispout[4]{};
    bool rl{};
    bool rt{};
    bool latchk1{};
    bool latchk2{};
    unsigned sigma{};
    bool carry{};
    MicroInstruction u_command{};
    std::uint32_t command{};
    unsigned char asp{};
    unsigned char cur_ucmd{};
    bool was_t_qrd{};
    unsigned icount{};
    unsigned dcount{};
    unsigned ecount{};
    unsigned ucount{};
    unsigned char cptr{};
};
} // namespace pmk

