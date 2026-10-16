#include "calculator_io.hpp"
#include "vfd_reference.hpp"

#include <iostream>

int main() {
    using namespace pmk::hardware;

    // Check the factual manual transcription on every core CI platform,
    // independently of Qt and of the implementation's decoder table.
    for (unsigned code = 0; code < ManualGlyphElectrodes.size(); ++code) {
        const auto nibble = static_cast<std::uint8_t>(code);
        VfdTube scanned;
        scanned.acceptScan(true, 2, nibble, false);
        std::array<std::uint8_t, 12> codes;
        codes.fill(15);
        codes[0] = nibble;
        const auto steady = VfdTube::steadySnapshot(codes);
        for (unsigned electrode = 0; electrode < 7; ++electrode) {
            const bool expected = ManualGlyphElectrodes[code].find(
                char('A' + electrode)) != std::string_view::npos;
            if (((decodedVfdSegments(nibble) & (1U << electrode)) != 0) != expected ||
                (scanned.snapshot().cells[0].phosphor[electrode] != 0) != expected ||
                (steady.cells[0].phosphor[electrode] != 0) != expected) {
                std::cerr << "Manual VFD mismatch: code " << code
                          << ", electrode " << char('A' + electrode) << '\n';
                return 10;
            }
        }
    }

    KeyboardMatrix keyboard;
    keyboard.pressContact(3, 9);
    for (unsigned d = 0; d <= 10; ++d) {
        const auto lines = keyboard.sample(d, true);
        if (d == 10) {
            if (!lines.k1 || !lines.k2) {
                std::cerr << "row-3 keyboard contact did not drive K1+K2\n";
                return 1;
            }
        } else if (lines.k1 || lines.k2) {
            std::cerr << "keyboard contact appeared on the wrong D strobe\n";
            return 2;
        }
    }
    if (!keyboard.contactActive() ||
        !keyboard.sample(10, true).k1 || !keyboard.sample(10, true).k2) {
        std::cerr << "held keyboard contact did not survive repeated scans\n";
        return 3;
    }
    keyboard.releaseContact();
    if (keyboard.contactActive() || keyboard.sample(10, true).k1 ||
        keyboard.sample(10, true).k2) {
        std::cerr << "released keyboard contact remained electrically active\n";
        return 3;
    }
    if (!keyboard.sample(12, true).k2 || keyboard.sample(12, false).k2) {
        std::cerr << "D13/K2 fixed connection is incorrect\n";
        return 4;
    }

    AngleSelector angle;
    angle.setPosition(AngleSelector::Position::Grads);
    if (angle.inputLevel(11) || !angle.inputLevel(9) || !angle.inputLevel(10)) {
        std::cerr << "angle-selector active-low scan is incorrect\n";
        return 5;
    }

    VfdTube tube;
    tube.acceptScan(true, 2, 14, false);       // E in first electrical cell
    tube.acceptScan(true, 3, 13, false);       // Г in second electrical cell
    tube.acceptScan(true, 4, 0 | 0x80U, true); // 0. and frame sync
    if (tube.codes()[0] != 14 || tube.codes()[1] != 13 ||
        tube.codes()[2] != (0 | 0x80U) || tube.snapshot().completedScans != 1) {
        std::cerr << "VFD did not consume the microcode scan bus\n";
        return 6;
    }
    const auto eMask = decodedVfdSegments(14);
    for (unsigned segment = 0; segment < 7; ++segment) {
        const bool charged = tube.snapshot().cells[0].phosphor[segment] != 0;
        if (charged != ((eMask & (1U << segment)) != 0)) {
            std::cerr << "VFD decoder/excitation mismatch\n";
            return 7;
        }
    }
    if (tube.snapshot().cells[2].phosphor[7] == 0) {
        std::cerr << "VFD decimal electrode was not excited\n";
        return 8;
    }
    const auto beforeDecay = tube.snapshot().cells[0].phosphor[0];
    tube.advanceMicrotick();
    const auto afterDecay = tube.snapshot().cells[0].phosphor[0];
    if (afterDecay >= beforeDecay || afterDecay == 0) {
        std::cerr << "VFD phosphor persistence is not decaying deterministically\n";
        return 9;
    }

    tube.acceptScan(false, 0, 0, true);
    for (const auto code : tube.codes()) {
        if (code != 0x0fU) {
            std::cerr << "disabled VFD frame was not blanked at SYNC\n";
            return 10;
        }
    }

    std::array<std::uint8_t, 12> steadyCodes;
    steadyCodes.fill(0x0fU);
    steadyCodes[VfdTube::electricalCell(0)] = 2 | 0x80U;
    const auto steady = VfdTube::steadySnapshot(steadyCodes);
    const auto& leftmost = steady.cells[VfdTube::electricalCell(0)];
    if (leftmost.code != (2 | 0x80U) || leftmost.phosphor[7] == 0 ||
        leftmost.phosphor[0] == 0) {
        std::cerr << "steady stack snapshot did not use the VFD electrode decoder\n";
        return 11;
    }

    return 0;
}
