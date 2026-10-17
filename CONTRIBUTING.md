# Contributing

Bug reports, verified calculator behavior, reference photographs, and focused
code changes are welcome. Please keep the interface faithful to the physical
Elektronika MK-61 and preserve the microcode-based execution model.

## Build and test

The complete Linux checks require Qt 6.4+ Widgets, Test and the SVG icon runtime
(`qt6-base-dev` and `libqt6svg6` on Debian/Ubuntu). From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Document the source of expected calculator behavior or visual measurements.
Do not add ROMs, fonts, photographs, or other third-party material without a
clear redistribution basis. By contributing, you agree that your contribution
may be distributed under GPL-3.0.

The portable core needs no Qt. The CI-style standalone build also checks
warnings and installation of the reusable libraries:

```sh
cmake -S . -B build-core -DMK61_BUILD_GUI=OFF \
  -DMK61_INSTALL_DEVELOPMENT=ON -DBUILD_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build-core --config Release --parallel 2
ctest --test-dir build-core -C Release -L core --output-on-failure --no-tests=error
cmake --install build-core --config Release --prefix build-core/install
```

CI runs portable core checks on Ubuntu, Windows and macOS. GUI builds register
a startup smoke test on each platform; detailed interaction/rendering checks
remain Linux-only. Native Windows/macOS package startup is checked separately
with SDK environment paths removed. These checks do not establish pixel fidelity
or replace interactive desktop and file-dialog testing. See [test coverage](tests/README.md)
and [packaging](packaging/README.md) for the scope of each check.

## Reuse the core

Configure with `MK61_BUILD_GUI=OFF` and `MK61_INSTALL_DEVELOPMENT=ON`, then install
to the prefix of your choice. Set a consumer's `CMAKE_PREFIX_PATH` to that prefix:

```cmake
find_package(MK61 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE MK61::Core)
```

The package supplies the C++20 requirement and hardware-library dependency.
Development files are excluded from the normal desktop installation.

The core advances by explicit microticks. `pmk::RealtimeClock` converts monotonic
elapsed nanoseconds to the 168 kHz target and carries fractional ticks between
calls. It caps each elapsed interval at 100 ms to bound catch-up after suspension;
the adapter itself reads no system clock. Frontends should advance the core
before changing input contacts. See `src/core/realtime_clock.hpp`.

Keyboard contacts are sampled once per four-bit input word, with no event queue.
A contact not sampled is not replayed later; the processor microcode determines
which sampled keys are acted on. Preserve this distinction when changing input
adapters. See `src/hardware/calculator_io.hpp`.

## Native state compatibility

`.mk61state` preserves processor registers, circulating memory, program, microtick
phase and VFD state. Its versioned little-endian format includes ROM identification
and a corruption checksum; invalid files are rejected before replacing state.
Version 2 also records the keyboard word sampled for the current four-bit cycle.
Version 1 files remain readable but cannot restore that previously unrecorded
word exactly; subsequent saves use version 2.

UI loading restores calculator contents and angle mode, powers the interface on
and starts desktop contacts released. It retains the current window layout.
Android Java `.pmk` states are not supported; mnemonic text import/export is a
separate interface. See `src/core/core_state.cpp` and the state round-trip tests.

## Source installation details

`./install.sh` uses `build-release/`, separate from manual `build/` configurations.
It installs the executable to `~/.local/bin/elektronika-mk61`, desktop entry to
`~/.local/share/applications`, icon to `~/.local/share/icons/hicolor` and notices
to `~/.local/share/doc/elektronika-mk61`. Cache helpers run when available.

For a custom absolute prefix, use `./install.sh /opt/elektronika-mk61` and the
same prefix with `./uninstall.sh /opt/elektronika-mk61`; administrator privileges
may be needed. The desktop launcher is generated with the installation prefix.
The equivalent standard installation of an existing manual build is:

```sh
cmake --install build --prefix "$HOME/.local"
```

Use these scripts only for source-based installations, not package-manager
installations. Windows/macOS deployment and native package generation are
documented in [Desktop packaging](packaging/README.md).

## Upstream references

The native core derives from EMU145 commit
`02a8286c8e839c4e325982ff4967e89025c6e626`. The PMK-Android behavioral/interface
reference is commit `100eea1e325fcf85f0d029780b9c079793a73075`. These references
do not establish independent physical-chip extraction or upstream endorsement.
Preserve the notices and provenance qualifications in [NOTICE](NOTICE).
