/*
 * Adapted from EMU145 by Felix Lazarev.
 * Emulator code GPL-3.0; ROM content copyright (c) 198x Trokhimenko Y.K. and others.
 */
#pragma once

#include <cstdint>

namespace pmk {
constexpr unsigned MCU_BITLEN = 168;

union MicroInstruction {
    struct {
        unsigned a_r:1, a_m:1, a_st:1, a_nr:1, a_10nl:1, a_s:1, a_4:1;
        unsigned b_s:1, b_ns:1, b_s1:1, b_6:1, b_1:1;
        unsigned g_l:1, g_nl:1, g_nt:1;
        unsigned r0:3, r_1:1, r_2:1, m:1, l:1;
        unsigned s:2, s1:2, st:2, pad:4;
    } bits;
    unsigned raw;
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
    unsigned cmdrom[256]{};

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
    unsigned command{};
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

