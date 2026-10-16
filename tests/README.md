# Verification

Configure from the repository root with `-DBUILD_TESTING=ON`. Core-only builds
use `-DMK61_BUILD_GUI=OFF` and require neither Qt nor external reference files.
Run `ctest --test-dir build-core -C Release -L core --output-on-failure --no-tests=error`.
Use `-DCMAKE_COMPILE_WARNING_AS_ERROR=ON` to reject compiler warnings.

Five portable core checks cover microcode execution and program-text
round trips, manual-derived programs (including branches/registers/stack),
signal-level input/output, clock batching, and full-state round trips and
invalid-state rejection. Tests fail explicitly, not through release-disabled
assertions. Matrix/semantics probes are diagnostic executables, not assertions
registered with CTest.

Linux Qt builds additionally register nine offscreen GUI/render checks,
including expanded-stack input, file-state actions, electrode rendering,
startup and HiDPI rendering. Use `ctest --test-dir build -C Release --output-on-failure`.
These tests are not enabled for Windows/macOS GUI builds.

Behavioral program sources are cited next to each fixture in
`program_regression.cpp`. The VFD reference in `vfd_reference.hpp` transcribes
factual electrode states from Table 1 of the supplied MK-61 operating manual;
its edition, pages and document checksum are recorded there. Original manual
images are not redistributed. Decoder output, scan excitation and rendered
electrodes are checked independently of the production truth table. The core
hardware test checks both scanned and steady electrodes on every platform. A real
division-by-zero sequence checks the error frame through the microcode.

CI results on each host establish only the checks actually run there. Building
the Qt-independent core on Windows/macOS does not establish desktop GUI
support. Visual fidelity still requires physical-reference and native desktop
inspection; these checks do not constitute a transistor-level hardware model.
