/*
 * Text command names and formatting adapted from PMK-Android CommandParser
 * and SaveStateManager, GPL-3.0.
 */
#include "program_text.hpp"

#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace pmk {
namespace {
struct Command {
    std::uint8_t code;
    std::uint8_t type; // 0 ordinary, 1 register suffix, 2 next byte is address
    std::string_view mnemonic;
};

constexpr std::array<Command, 91> Commands = {{
    {0x15, 0, "F10ˣ"}, {0x54, 0, "KНОП"}, {0x1F, 0, "1F"},
    {0x2F, 0, "2F"}, {0x56, 0, "K2"}, {0x16, 0, "Feˣ"},
    {0x17, 0, "Flg"}, {0x18, 0, "Fln"}, {0x30, 0, "KЧ→М"},
    {0x19, 0, "Fsinᐨ¹"}, {0x31, 0, "K|x|"}, {0x1A, 0, "Fcosᐨ¹"},
    {0x32, 0, "Kзн"}, {0x1B, 0, "Ftgᐨ¹"}, {0x33, 0, "KГ→М"},
    {0x1C, 0, "Fsin"}, {0x34, 0, "K[x]"}, {0x1D, 0, "Fcos"},
    {0x35, 0, "K{x}"}, {0x1E, 0, "Ftg"}, {0x36, 0, "Kmax"},
    {0x10, 0, "+"}, {0x11, 0, "-"}, {0x12, 0, "×"},
    {0x13, 0, "÷"}, {0x20, 0, "Fπ"}, {0x26, 0, "KМ→Г"},
    {0x21, 0, "F√"}, {0x22, 0, "Fx²"}, {0x23, 0, "F1/x"},
    {0x14, 0, "<->"}, {0x0E, 0, "В↑"}, {0x3D, 0, "3D"},
    {0x3C, 0, "3C"}, {0x3E, 0, "x←y"}, {0x3F, 0, "3F"},
    {0x24, 0, "Fxʸ"}, {0x27, 0, "K-"}, {0x28, 0, "K×"},
    {0x29, 0, "K÷"}, {0x2A, 0, "KМ→Ч"}, {0x2B, 0, "2B"},
    {0x2C, 0, "2C"}, {0x2D, 0, "2D"}, {0x2E, 0, "2E"},
    {0x0F, 0, "FВх"}, {0x3B, 0, "KCЧ"},
    {0x0A, 0, "."}, {0x0B, 0, "/-/"}, {0x0C, 0, "ВП"},
    {0x0D, 0, "Сx"}, {0x25, 0, "Fѻ"}, {0x37, 0, "KɅ"},
    {0x38, 0, "KV"}, {0x39, 0, "K(+)"}, {0x3A, 0, "Kинв"},
    {0x52, 0, "В/О"}, {0x50, 0, "С/П"}, {0x59, 2, "Fx≥0"},
    {0x57, 2, "Fx≠0"}, {0x51, 2, "БП"}, {0x53, 2, "ПП"},
    {0x58, 2, "FL2"}, {0x5A, 2, "FL3"}, {0x5C, 2, "Fx<0"},
    {0x5E, 2, "Fx=0"}, {0x5D, 2, "FL0"}, {0x5B, 2, "FL1"},
    {0x5F, 0, "5F"}, {0x40, 1, "xП"}, {0x4F, 0, "4F"},
    {0x60, 1, "Пx"}, {0x6F, 0, "6F"}, {0x70, 1, "Kx≠0"},
    {0x7F, 0, "7F"}, {0x80, 1, "KБП"}, {0x8F, 0, "8F"},
    {0x90, 1, "Kx≥0"}, {0x9F, 0, "9F"}, {0xA0, 1, "KПП"},
    {0xAF, 0, "AF"}, {0xB0, 1, "KxП"}, {0xBF, 0, "BF"},
    {0xC0, 1, "Kx<0"}, {0xCF, 0, "CF"}, {0xD0, 1, "KПx"},
    {0xDF, 0, "DF"}, {0xE0, 1, "Kx=0"}, {0xEF, 0, "EF"},
    {0xF0, 1, "F"}, {0xFF, 0, "FF"}
}};

std::string hex(std::uint8_t value, bool addressOperand = false) {
    std::ostringstream out;
    out << std::uppercase << std::hex;
    if (addressOperand && value < 10)
        out << '0';
    out << static_cast<unsigned>(value);
    return out.str();
}

std::string mnemonic(std::uint8_t code, bool& isAddress) {
    if (isAddress) {
        isAddress = false;
        return hex(code, true);
    }
    for (const auto& command : Commands) {
        if (command.type == 1) {
            const unsigned reg = code & 0x0fU;
            if (reg < 0x0fU && (code & 0xf0U) == command.code) {
                return std::string(command.mnemonic) + hex(reg);
            }
        } else if (command.code == code) {
            isAddress = command.type == 2;
            return std::string(command.mnemonic);
        }
    }
    return hex(code);
}

int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

std::uint8_t parseMnemonic(std::string_view token) {
    for (const auto& command : Commands) {
        if (command.type == 1) {
            if (token.size() == command.mnemonic.size() + 1 &&
                token.starts_with(command.mnemonic)) {
                const int reg = hexDigit(token.back());
                if (reg >= 0 && reg < 15)
                    return static_cast<std::uint8_t>(command.code + reg);
            }
        } else if (token == command.mnemonic) {
            return command.code;
        }
    }
    unsigned value = 0;
    const auto [end, error] =
        std::from_chars(token.data(), token.data() + token.size(), value, 16);
    if (error != std::errc{} || end != token.data() + token.size() ||
        value > 0xffU)
        throw std::invalid_argument("unknown PMK command: " + std::string(token));
    return static_cast<std::uint8_t>(value);
}
} // namespace

std::string exportProgramText(const CalculatorCore::Program& program) {
    std::ostringstream out;
    bool isAddress = false;
    for (std::size_t address = 0; address < program.size(); ++address) {
        if (address < 10) out << '0';
        if (address < 100) out << address;
        else out << "A0"; // Preserve PMK-Android SaveStateManager behavior.
        out << '.' << mnemonic(program[address], isAddress) << '\t';
        if ((address + 1) % 10 == 0)
            out << '\n';
    }
    return out.str();
}

ParsedProgram importProgramText(std::string_view text) {
    std::string normalized;
    std::istringstream lines{std::string(text)};
    std::string line;
    while (std::getline(lines, line)) {
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#')
            continue;
        for (std::size_t i = 0; i < line.size(); ++i) {
            const auto whitespace = [](char c) {
                return std::isspace(static_cast<unsigned char>(c)) != 0;
            };
            if (line[i] == '|' && i != 0 && i + 1 < line.size() &&
                whitespace(line[i - 1]) && whitespace(line[i + 1])) {
                while (!normalized.empty() && whitespace(normalized.back()))
                    normalized.pop_back();
                normalized += '.';
                while (i + 1 < line.size() && whitespace(line[i + 1]))
                    ++i;
            } else {
                normalized += line[i];
            }
        }
        normalized += ' ';
    }

    ParsedProgram result;
    std::istringstream tokens(normalized);
    std::string token;
    while (tokens >> token) {
        if (result.size == result.bytes.size())
            throw std::length_error("PMK program exceeds 105 commands");
        if (token.size() > 2 && token[2] == '.')
            token.erase(0, 3);
        try {
            result.bytes[result.size++] = parseMnemonic(token);
        } catch (const std::exception&) {
            throw std::invalid_argument("unknown PMK command at " +
                                        std::to_string(result.size) + ": " + token);
        }
    }
    return result;
}

} // namespace pmk
