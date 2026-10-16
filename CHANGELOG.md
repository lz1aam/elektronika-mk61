# Changelog

## 0.1.2

- Keep each sampled keyboard word stable through its four serial bit phases,
  preventing corrupted results from rapid contacts without buffering keys.
- Extend native states to version 2 for exact sampled-input restoration;
  retain version 1 import with a documented input-word migration.
- Focus the application and core exclusively on MK-61; remove MK-54 support.
- Optimize serial delay memories using circular buffers, preserving logical
  ordering and the MK-61 processor chain.
- Replace compiler-dependent microinstruction bitfields with fixed-width
  decoding while retaining the existing numeric ROM tables.
- Separate the Qt-independent core and hardware libraries from the desktop
  build; provide optional installable CMake targets for other frontends.
- Add a platform-independent elapsed-time adapter with fractional-tick carry
  and bounded catch-up after host suspension.
- Save and resume complete native calculator states in `.mk61state` files,
  with version, ROM identity and checksum validation before loading.
- Improve keyboard contact ownership and input timing; harden text-program
  parsing and address validation.
- Correct display code D to use the top and full left side, producing the
  documented EГГ0Г error without changing display geometry or colors.
- Update MK-61 desktop metadata and install the bundled DejaVu font licence.

The desktop application is validated locally on Linux. Windows/macOS and
other CPU architectures still require native validation and deployment work.
Android Java `.pmk` state files are not supported. Historical ROM provenance
limitations remain documented in [NOTICE](NOTICE).

## 0.1.1

- Improve physical keyboard timing and desktop application lifecycle handling.
