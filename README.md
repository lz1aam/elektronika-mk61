# Elektronika MK-61 Emulator

## Download for Linux, Windows and macOS

[Download v0.1.2 test packages](https://github.com/lz1aam/elektronika-mk61/releases/tag/v0.1.2)
— choose your OS and CPU from the download table. No compilation is needed.

This is a **pre-release for community testing**, not a stable release. Read the
installation prerequisites and known security/signing limitations on that page.
[Report test results or problems](https://github.com/lz1aam/elektronika-mk61/issues/new).

Native Linux emulation of the Soviet **Elektronika MK-61** programmable RPN
calculator, written in C++20 and Qt 6.

Current source version: **0.1.2**. See [release notes](CHANGELOG.md).

The calculator core is a separate C++20 library with no Qt or Linux API
dependency. The desktop application currently targets Linux. Windows/macOS
desktop deployment is being validated separately from the portable core.

| Original display | Expanded four-row VFD stack |
|:--:|:--:|
| <img src="resources/screenshots/elektronika-mk61.png" alt="Elektronika MK-61 Emulator" width="360"> | <img src="resources/screenshots/elektronika-mk61-stack.png" alt="Elektronika MK-61 Emulator with expanded T/Z/Y/X display" width="360"> |

The application executes the original IK13xx microcode model instead of using
modern floating-point calculator logic. Its keyboard matrix, angle selector,
and ИЛЦ2-12/8Л vacuum-fluorescent display are connected to that emulated
hardware state. This is an independent, work-in-progress port; it is not an
official release of PMK-Android, EMU145, or the original manufacturer.

## Current features

- Native Qt Widgets interface with mouse and keyboard input.
- MK-61 panel and calculator behavior.
- Authentic microcode execution, RPN stack, registers, and program memory.
- Hardware-driven VFD rendering and optional four-row `T/Z/Y/X` stack view.
- Import and export of PMK-Android-style mnemonic text programs.
- Save and resume complete native calculator states (`.mk61state`).
- Linux desktop launcher and scalable application icon.

Only MK-61 is supported; there is no MK-54 mode or model selector.

The native core is derived from EMU145 commit
`02a8286c8e839c4e325982ff4967e89025c6e626`; the behavioral and interface
reference is PMK-Android commit
`100eea1e325fcf85f0d029780b9c079793a73075`.

## Build

Requirements: CMake 3.20+, a C++20 compiler, Qt 6 Widgets development
files (`qt6-base-dev` on Debian/Ubuntu), and the Qt SVG runtime module
(`libqt6svg6`) for the application icon.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/elektronika-mk61
```

To build only the core, without Qt:

```sh
cmake -S . -B build-core -DMK61_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --config Release
```

For use in another CMake project, also configure with
`-DMK61_INSTALL_DEVELOPMENT=ON`, then install to your chosen prefix. Consumers
can use `find_package(MK61 CONFIG REQUIRED)` and link `MK61::Core`; the package
provides its C++20 requirement and hardware-library dependency automatically.
Set `CMAKE_PREFIX_PATH` to that installation prefix when configuring a consumer.
Development files are excluded from the normal desktop installation.

The core advances by explicit microticks. An optional `pmk::RealtimeClock`
adapter converts monotonic elapsed nanoseconds to the 168 kHz target and
carries fractional ticks between calls. It caps each elapsed interval at
100 ms to bound work after a host suspension. Frontends should advance the
core before changing input contacts. The adapter itself reads no system clock.

Build configurations cover GCC/Clang and MSVC. Linux desktop integration is
enabled only on Linux; Qt targets use a Windows GUI executable or macOS app
bundle on those systems. Windows/macOS deployment and ARM execution must be
verified by their package jobs; core-only checks do not establish GUI support.

## Verification

Portable core checks require no Qt:

```sh
cmake -S . -B build-core -DMK61_BUILD_GUI=OFF -DBUILD_TESTING=ON -DMK61_INSTALL_DEVELOPMENT=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build-core --config Release --parallel 2
ctest --test-dir build-core -C Release -L core --output-on-failure --no-tests=error
cmake --install build-core --config Release --prefix core-install
```

CI is configured to build, run these checks and install the development
package on Ubuntu, Windows and macOS. Remote platform results must be checked
after the workflow runs. Detailed Qt GUI checks remain Linux-only and require
Qt 6 Test as well as Widgets (both supplied by `qt6-base-dev`); GUI startup
smoke tests are also registered for Windows/macOS when the app is built:

```sh
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build -C Release --output-on-failure --no-tests=error
```

GUI checks use Qt's offscreen platform; they do not establish native desktop
or visual fidelity on other operating systems. Set `-DBUILD_TESTING=OFF` to
build only the application or libraries. See [test notes](tests/README.md)
for coverage and reference provenance.

## Installation

DEB/RPM, Flatpak, Windows ZIP/installer and macOS DMG preparation and validation
are described in [Desktop packages](packaging/README.md). CI artifacts are
review builds, not signed releases or proof of support for every distribution.

Install for the current user:

```sh
./install.sh
```

The installer keeps its optimized build in `build-release/`, independently
of the manual `build/` directory shown above.

This installs the executable as `~/.local/bin/elektronika-mk61`, the launcher
under `~/.local/share/applications`, and the icon under
`~/.local/share/icons/hicolor`. License and attribution files are installed
under `~/.local/share/doc/elektronika-mk61`, including the bundled font licence.
The desktop and icon caches are refreshed
automatically when the corresponding cache utilities are available. The
launcher follows the freedesktop/XDG layout and does not depend on GNOME, KDE
Plasma, XFCE, LXQt, or another particular desktop environment. Menus normally
notice the new entry automatically; if a running session keeps an old menu
cache, sign out and back in once.

The application can always be started directly with:

```sh
"$HOME/.local/bin/elektronika-mk61"
```

The launcher is generated during installation, so its absolute executable
path follows the selected prefix. To install under another absolute prefix,
pass it as the first argument, for example `./install.sh /opt/elektronika-mk61`
(a system prefix may require administrator privileges).

Uninstall the current-user installation with:

```sh
./uninstall.sh
```

Both scripts are desktop-independent convenience wrappers. The equivalent
standard CMake installation remains available:

```sh
cmake --install build --prefix "$HOME/.local"
```

## Controls

Digits, decimal point/comma, Enter, `+`, `-`, `*`, `/`, Backspace/Escape,
`F`, and `K` map to calculator keys. `R`, `G`, and `D` select radians, grads,
and degrees; `F12` toggles power. Click the powered display to toggle the
four-row stack view.

Click the printed `ВКЛ` caption beside the power switch to open the file menu.
Use **Save calculator state…** and **Load calculator state…** for `.mk61state`
files. Loading restores the machine and angle switch and powers the interface
on; desktop keys start released. The current window layout is retained.

Native states preserve processor registers, circulating memory, program,
microtick phase and VFD state. Their versioned little-endian format includes
ROM identification and a corruption checksum; invalid files are rejected
before replacing the calculator state. Version 2 also preserves the keyboard
word sampled for the current four-bit cycle. Version 1 files can be loaded,
but cannot exactly restore that previously unrecorded input word; they are
saved as version 2 afterward. These files are distinct from Android
Java `.pmk` files, which are not currently supported. Text program import and
export remain separate menu actions.

Keyboard contacts are sampled once per four-bit input word, with no key-event
queue. A contact that is not sampled is not replayed later; the processor
microcode still determines which sampled keys are acted on.

## License

The project is licensed under the [GNU GPL version 3](LICENSE). It contains
adapted GPL-3.0 code and assets from
[EMU145](https://github.com/fixelsan/emu145) and
[PMK-Android](https://github.com/xvadim/pmk-android). EMU145's ROM-bearing files
carry GPL-3.0 headers, while their older chain of title is not independently
documented; read [NOTICE](NOTICE) before distributing binaries. Contributions
are described in [CONTRIBUTING.md](CONTRIBUTING.md).
