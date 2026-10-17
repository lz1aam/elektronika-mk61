#include "calculator_core.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace pmk {
namespace {
constexpr std::array<std::uint8_t, 8> Magic = {'M','K','6','1','S','T','A','T'};
constexpr std::uint16_t Version = 2;
constexpr std::size_t HeaderBytes = 20;

void require(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}

std::uint32_t crc32(std::span<const std::uint8_t> data) {
    std::uint32_t crc = 0xffffffffU;
    for (auto byte : data) {
        crc ^= byte;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xedb88320U : 0U);
    }
    return crc ^ 0xffffffffU;
}

struct Writer {
    std::vector<std::uint8_t> data;
    template<typename T> void byte(T value) {
        data.push_back(static_cast<std::uint8_t>(value));
    }
    void boolean(bool value) { byte(value ? 1 : 0); }
    void u16(std::uint16_t value) {
        for (unsigned i = 0; i < 16; i += 8) byte(value >> i);
    }
    void u32(std::uint32_t value) {
        for (unsigned i = 0; i < 32; i += 8) byte(value >> i);
    }
    void u64(std::uint64_t value) {
        for (unsigned i = 0; i < 64; i += 8) byte(value >> i);
    }
};

struct Reader {
    std::span<const std::uint8_t> data;
    std::size_t offset{};
    template<typename T> void byte(T& value) {
        require(offset < data.size(), "truncated MK-61 state");
        value = static_cast<T>(data[offset++]);
    }
    void boolean(bool& value) {
        std::uint8_t encoded{};
        byte(encoded);
        require(encoded <= 1, "invalid state boolean");
        value = encoded != 0;
    }
    void u16(std::uint16_t& value) {
        value = 0;
        for (unsigned i = 0; i < 16; i += 8) {
            std::uint8_t b{}; byte(b);
            value = static_cast<std::uint16_t>(value | (std::uint16_t(b) << i));
        }
    }
    void u32(std::uint32_t& value) {
        value = 0;
        for (unsigned i = 0; i < 32; i += 8) {
            std::uint8_t b{}; byte(b); value |= std::uint32_t(b) << i;
        }
    }
    void u64(std::uint64_t& value) {
        value = 0;
        for (unsigned i = 0; i < 64; i += 8) {
            std::uint8_t b{}; byte(b); value |= std::uint64_t(b) << i;
        }
    }
};

template<typename Archive, typename Chip>
void chipFields(Archive& archive, Chip& chip) {
    for (auto& bit : chip.rm) archive.boolean(bit);
    for (auto& bit : chip.rr) archive.boolean(bit);
    for (auto& bit : chip.rst) archive.boolean(bit);
    for (auto& bit : chip.rs) archive.boolean(bit);
    for (auto& bit : chip.rs1) archive.boolean(bit);
    for (auto& bit : chip.rh) archive.boolean(bit);
    for (auto& bit : chip.dispout) archive.boolean(bit);
    archive.boolean(chip.rl); archive.boolean(chip.rt);
    archive.boolean(chip.latchk1); archive.boolean(chip.latchk2);
    archive.byte(chip.sigma); archive.boolean(chip.carry);
    archive.u32(chip.u_command.raw); archive.u32(chip.command);
    archive.byte(chip.asp); archive.byte(chip.cur_ucmd);
    archive.boolean(chip.was_t_qrd);
    archive.byte(chip.icount); archive.byte(chip.dcount);
    archive.byte(chip.ecount); archive.byte(chip.ucount); archive.byte(chip.cptr);
}

void validateChip(const MicrocodeChip& chip) {
    require(chip.sigma <= 1 && chip.asp < 128 && chip.cur_ucmd < 68 &&
            chip.icount < 42 && chip.dcount < 14 && chip.ecount < 3 &&
            chip.ucount < 4, "invalid processor counters in state");
    require(chip.command == chip.cmdrom[chip.cptr] &&
            chip.u_command.raw == chip.ucrom[chip.cur_ucmd].raw,
            "invalid processor instruction in state");
}

template<typename Archive, typename Snapshot>
void tubeFields(Archive& archive, Snapshot& tube) {
    archive.u64(tube.completedScans);
    for (auto& cell : tube.cells) {
        archive.byte(cell.code);
        for (auto& charge : cell.phosphor) archive.u16(charge);
    }
}

void validateTube(const hardware::VfdSnapshot& tube) {
    for (const auto& cell : tube.cells)
        require((cell.code & 0x70U) == 0, "invalid VFD code in state");
}

void writeMemory(Writer& out, const DelayMemory& memory) {
    for (unsigned i = 0; i < DelayMemory::BitLength / 4; ++i)
        out.byte(memory.nibbleFromHead(i));
}

void readMemory(Reader& in, DelayMemory& memory) {
    memory.reset();
    for (unsigned i = 0; i < DelayMemory::BitLength / 4; ++i) {
        std::uint8_t value{}; in.byte(value);
        require(value < 16, "invalid serial-memory nibble in state");
        memory.setNibbleFromHead(i, value);
    }
}
} // namespace

struct CoreStateCodec {
    // FNV-1a over numeric ROM words encoded little-endian; ROM contents are
    // identified, never embedded or replaced by a state file.
    static std::uint64_t romIdentity(const CalculatorCore& core) {
        Writer words;
        for (const auto* chip : {&core.ik1302_, &core.ik1303_, &core.ik1306_}) {
            for (const auto& word : chip->ucrom) words.u32(word.raw);
            for (const auto& row : chip->asprom)
                for (auto byte : row) words.byte(byte);
            for (auto word : chip->cmdrom) words.u32(word);
        }
        std::uint64_t hash = 14695981039346656037ULL;
        for (auto byte : words.data) {
            hash ^= byte;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    static std::vector<std::uint8_t> save(const CalculatorCore& core) {
        Writer payload;
        payload.u64(romIdentity(core));
        chipFields(payload, core.ik1302_);
        chipFields(payload, core.ik1303_);
        chipFields(payload, core.ik1306_);
        writeMemory(payload, core.ir2_1_);
        writeMemory(payload, core.ir2_2_);
        payload.boolean(core.chain_); payload.boolean(core.sync_);
        payload.byte(core.dcycle_); payload.byte(core.mode_);
        payload.u64(core.ticks_); payload.byte(core.memorySyncCounter_);
        payload.u16(core.hardware_.keyboard().encodedContact_);
        const auto& tube = core.hardware_.display();
        tubeFields(payload, tube.snapshot_);
        for (auto code : tube.codes_) payload.byte(code);
        payload.boolean(tube.frameDriven_);
        for (const auto& row : core.stackDisplaySnapshots_) tubeFields(payload, row);
        payload.boolean(core.hardware_.keyboard().sampledLines_.k1);
        payload.boolean(core.hardware_.keyboard().sampledLines_.k2);

        Writer out;
        out.data.insert(out.data.end(), Magic.begin(), Magic.end());
        out.u16(Version); out.u16(0); // reserved flags
        out.u32(static_cast<std::uint32_t>(payload.data.size()));
        out.u32(crc32(payload.data));
        out.data.insert(out.data.end(), payload.data.begin(), payload.data.end());
        return out.data;
    }

    static CalculatorCore load(std::span<const std::uint8_t> bytes) {
        require(bytes.size() >= HeaderBytes &&
                bytes.size() <= CalculatorCore::MaximumStateBytes,
                "invalid MK-61 state size");
        require(std::equal(Magic.begin(), Magic.end(), bytes.begin()),
                "not a native MK-61 state file");
        Reader header{bytes.subspan(Magic.size())};
        std::uint16_t version{}, flags{};
        std::uint32_t size{}, checksum{};
        header.u16(version); header.u16(flags);
        header.u32(size); header.u32(checksum);
        require((version == 1 || version == Version) && flags == 0,
                "unsupported MK-61 state version");
        const auto payload = bytes.subspan(HeaderBytes);
        require(size == payload.size(), "MK-61 state length mismatch");
        require(checksum == crc32(payload), "MK-61 state checksum mismatch");

        CalculatorCore core;
        Reader in{payload};
        std::uint64_t identity{}; in.u64(identity);
        require(identity == romIdentity(core), "MK-61 state uses different ROM tables");
        chipFields(in, core.ik1302_); validateChip(core.ik1302_);
        chipFields(in, core.ik1303_); validateChip(core.ik1303_);
        chipFields(in, core.ik1306_); validateChip(core.ik1306_);
        readMemory(in, core.ir2_1_); readMemory(in, core.ir2_2_);
        in.boolean(core.chain_); in.boolean(core.sync_);
        in.byte(core.dcycle_);
        unsigned mode{}; in.byte(mode);
        require(mode <= 2, "invalid angle mode in state");
        core.setAngleMode(static_cast<AngleMode>(mode));
        in.u64(core.ticks_); in.byte(core.memorySyncCounter_);
        require(core.dcycle_ <= 14 && core.memorySyncCounter_ < 5,
                "invalid machine phase in state");
        auto& keyboard = core.hardware_.keyboard();
        in.u16(keyboard.encodedContact_);
        const unsigned row = keyboard.encodedContact_ >> 8;
        const unsigned column = keyboard.encodedContact_ & 0xffU;
        require(keyboard.encodedContact_ == 0 ||
                (row >= 1 && row <= 3 && column <= 9), "invalid key contact in state");
        auto& tube = core.hardware_.display();
        tubeFields(in, tube.snapshot_); validateTube(tube.snapshot_);
        for (unsigned i = 0; i < tube.codes_.size(); ++i) {
            in.byte(tube.codes_[i]);
            require(tube.codes_[i] == tube.snapshot_.cells[i].code,
                    "inconsistent VFD codes in state");
        }
        in.boolean(tube.frameDriven_);
        for (auto& stack : core.stackDisplaySnapshots_) {
            tubeFields(in, stack); validateTube(stack);
        }
        if (version >= 2) {
            in.boolean(keyboard.sampledLines_.k1);
            in.boolean(keyboard.sampledLines_.k2);
        } else {
            // v1 did not store the sampled input word. Adopt its saved contact
            // as the initial word; exact mid-word continuation requires v2.
            keyboard.sample(core.ik1302_.dcount, core.ik1302_.strobe(), 0);
        }
        require(in.offset == payload.size(), "unexpected data in MK-61 state");
        return core;
    }
};

std::vector<std::uint8_t> CalculatorCore::saveState() const {
    return CoreStateCodec::save(*this);
}

void CalculatorCore::loadState(std::span<const std::uint8_t> data) {
    *this = CoreStateCodec::load(data);
}
} // namespace pmk
