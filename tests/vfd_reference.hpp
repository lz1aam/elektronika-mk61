// Factual segment transcription, independent of the implementation's decoder.
// Source: Elektronika MK-61 Calculator Operating Instructions, draft 1.0,
// Oct. 2014, Table 1, printed p. 19 / PDF p. 21. Error: printed p. 27.
// Supplied PDF SHA-256:
// 3c9f6b98e2419cc91052f73ba2d026ed414ad551f81db027afa89d6b73eeec7f
// Verified against the original glyph images, not our own rendered output.
// No original document images are redistributed with these factual records.
// Electrode names: A top, B upper right, C lower right, D bottom,
// E lower left, F upper left, G middle. Array index is the four-bit code.
#pragma once

#include <array>
#include <string_view>

inline constexpr std::array<std::string_view, 16> ManualGlyphElectrodes = {{
    "ABCDEF", "BC", "ABDEG", "ABCDG", "BCFG", "ACDFG", "ACDEFG", "ABC",
    "ABCDEFG", "ABCDFG", "G", "DEF", "ADEF", "AEF", "ADEFG", ""
}};
