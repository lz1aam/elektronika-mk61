# Elektronika MK-61 Emulator

A desktop emulator of the Soviet **Elektronika MK-61** programmable RPN
calculator, built with C++20 and Qt 6. It executes the IK13xx microcode model
rather than substituting modern floating-point calculator logic.

**[Download test packages](https://github.com/lz1aam/elektronika-mk61/releases)**
· [Changelog](CHANGELOG.md)
· [Report a problem](https://github.com/lz1aam/elektronika-mk61/issues/new)

The downloads are a **community-testing pre-release**, not a stable release.
Windows/macOS bundles are unsigned and include Qt 6.9.3 with known security
issues. Read the [prerequisites and limitations](https://github.com/lz1aam/elektronika-mk61/releases)
before testing; use trusted test files only.

| Original single-row VFD | Expanded four-row stack |
|:--:|:--:|
| <img src="resources/screenshots/elektronika-mk61.png" alt="MK-61 emulator with original single-row display" width="360"> | <img src="resources/screenshots/elektronika-mk61-stack.png" alt="MK-61 emulator with expanded T/Z/Y/X display" width="360"> |

## Features

- Microcode-based MK-61 execution: RPN stack, registers and keystroke programs.
- Qt Widgets panel with mouse and physical-keyboard input.
- Hardware-driven VFD rendering with an optional four-row `T/Z/Y/X` view.
- Import/export of PMK-Android-style mnemonic text programs.
- Save/resume of native calculator states (`.mk61state`).
- Offline calculator operation; no Android runtime, JVM or WebView.

## Download and install

[Open the downloads page](https://github.com/lz1aam/elektronika-mk61/releases)
and choose your OS and CPU architecture. No compilation is needed.

| Platform | Packages | Before starting |
|---|---|---|
| Linux | Ubuntu/Debian DEB, Fedora RPM, Flatpak | Choose the matching distribution/CPU; other distributions can try Flatpak. |
| Windows x64 | EXE installer or ZIP | Install the official VC++ x64 runtime linked on the release page if needed. Keep the complete ZIP directory. |
| macOS | Separate Intel and Apple Silicon DMGs | Copy the app to Applications. The preview is not signed or notarized. |

Installation commands, exact target distributions and known issues are on the
release page. These are project-built test packages, not packages accepted into
official operating-system repositories. Automated build/startup checks have
passed for the listed targets; native visual, input and file-dialog testing
still needs community feedback.

To help, [report your results](https://github.com/lz1aam/elektronika-mk61/issues/new)
with your OS version, CPU, package filename, display scaling and reproduction
steps. Successful tests are useful too.

## Using the calculator

Use the on-screen keys and switches, or these keyboard shortcuts:

| Input | Calculator action |
|---|---|
| `0–9`, `.` or `,` | Digits and decimal point |
| `Enter` | MK-61 enter key |
| `+`, `-`, `*`, `/` | Arithmetic keys |
| `Backspace` or `Escape` | Clear key |
| `F`, `K` | Function prefixes |
| `R`, `G`, `D` | Radians, grads, degrees |
| `F12` | Power on/off |

Click the display to switch between the original single-row view and the
four-row stack, even while powered off. Changing the view does not switch on
the display. `X` is at the bottom, with `Y`, `Z` and `T` above it.

Click the printed **ВКЛ** caption beside the power switch to open the file menu.
Program import/export and calculator-state save/load are separate actions.
Native `.mk61state` files restore calculator contents and the angle switch;
they are **not compatible with Android Java `.pmk` state files**.

## Build from source on Linux

Requirements: CMake 3.20+, a C++20 compiler and Qt 6.4+ Widgets development files.
On Debian/Ubuntu, install `build-essential`, `cmake`, `qt6-base-dev` and
`libqt6svg6` (the SVG icon runtime).

From a source checkout, the convenience installer builds and installs for the
current user:

```sh
./install.sh
```

It uses `build-release/` and installs under `~/.local`, including the desktop
launcher, icon and licenses. Desktop integration follows freedesktop/XDG
conventions and does not require a particular Linux desktop environment.
If a session keeps an old menu entry, sign out and back in.

Start the installed application directly with:

```sh
"$HOME/.local/bin/elektronika-mk61"
```

To remove this source-based installation, run `./uninstall.sh`.
Remove DEB/RPM/Flatpak packages through their package manager instead.

For a manual application-only build without installation:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build --config Release --parallel 2
./build/elektronika-mk61
```

## Development

The calculator core is a Qt-independent C++20 library. See:

- [Contributing and developer guide](CONTRIBUTING.md) — build/test commands,
  reusable core, timing and state-format notes.
- [Verification](tests/README.md) — coverage and reference provenance.
- [Desktop packaging](packaging/README.md) — native package builds, Qt deployment
  and platform limitations.

## License and credits

Licensed under the [GNU GPL version 3](LICENSE), with adapted code/assets from
[EMU145](https://github.com/fixelsan/emu145) and
[PMK-Android](https://github.com/xvadim/pmk-android). Bundled DejaVu fonts retain
their [own license](resources/fonts/DejaVu-LICENSE.txt).

Read [NOTICE](NOTICE) for attribution and historical ROM/font provenance
qualifications before redistributing. This is an independent project, not an
official release of or an endorsement by those upstream projects or the
original manufacturer.
