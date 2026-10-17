# Desktop packages

These are project-built packages, not packages accepted into official operating
system repositories. The `Packages` workflow creates review artifacts only; it
does not publish releases, sign binaries, or upload to Flathub. Check the
particular workflow results before claiming a platform is supported.

| Target | Format | Configured verification |
|---|---|---|
| Ubuntu 24.04, x86_64 / ARM64 | DEB | Full Linux suite; clean-container install, startup, reinstall, removal |
| Debian 12, x86_64 | DEB | Same checks, built against Debian's own libraries |
| Fedora 43, x86_64 | RPM | Same checks, built against Fedora's own libraries |
| Linux with Flatpak, x86_64 / ARM64 | Flatpak | Sandbox build, install, startup, removal |
| Windows, x86_64 | ZIP / NSIS EXE | Core and GUI startup; deployed/ZIP startup; silent install/removal |
| macOS, Intel / Apple Silicon | Separate DMGs | Core and GUI startup; deployed/mounted DMG startup |

Offscreen startup does not verify native windowing, scaling, keyboard input,
file dialogs or security prompts. Manually check these before public releases.
The silent Windows test does not replace an interactive installer review.
Windows ARM64, 32-bit systems, universal macOS bundles and AppImage are not
configured. Other Linux distributions can use Flatpak but are not all tested
individually. Flatpak does not require a KDE desktop.

## Native Linux builds

Install normal Qt build dependencies plus `dpkg-dev` (DEB) or `rpm-build` (RPM).
Build on the target distribution; converting an Ubuntu executable to RPM does
not make it compatible with Fedora.

```sh
cmake -S . -B build-package -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr -DMK61_ENABLE_PACKAGING=ON
cmake --build build-package --parallel 2
ctest --test-dir build-package --output-on-failure
cpack --config build-package/CPackConfig.cmake -G DEB
# On a matching RPM distribution, use -G RPM instead.
```

Output: `build-package/packages/`. Dependencies are detected from the executable,
with an explicit SVG plugin dependency. The default RPM SVG package name is
`qt6-qtsvg` (Fedora); other RPM families need their own name through
`MK61_RPM_QT_SVG_PACKAGE` and native validation. Use the system package manager
to install/remove packages, not `uninstall.sh`. An existing `~/.local` source
installation can take precedence in menus; remove that installation separately
with `./uninstall.sh` if necessary.

## Windows and macOS

Requires a C++20 compiler, CMake, matching Qt Widgets/SVG/image-formats SDK and
Python 3.11+. Windows also requires NSIS 3.03+. CI pins Qt 6.11.2 and aqtinstall
3.3.0. Qt libraries/plugins are dynamically deployed through Qt's deployment
tools. Native ICO/ICNS icons are rendered from the existing project SVG.

```sh
python3 packaging/prepare_qt_sources.py --version 6.11.2 \
  --sources build-package/packages/qt-sources --notices build-package/qt-notices
cmake -S . -B build-package -DCMAKE_BUILD_TYPE=Release \
  -DMK61_ENABLE_PACKAGING=ON -DMK61_DEPLOY_QT=ON \
  -DMK61_QT_NOTICES_DIR="<absolute Qt notices directory from above>" \
  -DCMAKE_PREFIX_PATH="<Qt SDK prefix>"
cmake --build build-package --config Release --parallel 2
ctest --test-dir build-package -C Release --output-on-failure
cmake --install build-package --config Release --prefix "<absolute staging path>"
python3 packaging/verify_install.py "<absolute staging path>"
cpack --config build-package/CPackConfig.cmake -C Release
```

Use `-A x64` with Visual Studio; select `arm64` or `x86_64` using
`CMAKE_OSX_ARCHITECTURES` on macOS. Build/deploy on the native host.
NSIS installs a Start Menu shortcut and uninstaller without modifying PATH.
For ZIP, retain the complete directory, not just the EXE. For DMG, copy the
complete app to Applications; delete the app to uninstall. Package removal
does not delete user-created calculator states.

## Flatpak

Install Flatpak, flatpak-builder and the KDE 6.11 Platform/SDK, then:

```sh
mkdir -p build-flatpak
git archive --format=tar.gz --prefix=elektronika-mk61/ \
  --output=build-flatpak/source.tar.gz HEAD
flatpak-builder --user --force-clean --repo=build-flatpak/repo \
  build-flatpak/app packaging/io.github.lz1aam.elektronika-mk61.json
flatpak build-bundle build-flatpak/repo build-flatpak/elektronika-mk61.flatpak \
  io.github.lz1aam.elektronika-mk61 \
  --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user build-flatpak/elektronika-mk61.flatpak
flatpak run io.github.lz1aam.elektronika-mk61
```

This builds committed source, not uncommitted edits. No network or blanket home
access is granted to the application. File dialogs should use desktop portals;
interactive save/load validation remains required.

## Source, notices and release gate

CI includes the corresponding application source archive from the exact built
commit. Windows/macOS artifacts additionally include official matching Qt
source archives for qtbase, qtsvg and qtimageformats, verified against published
SHA256 files. Qt license/copyright notices and source links are installed with
the app. Preserve corresponding sources and notices when redistributing; CI
artifact retention is not a long-term source distribution service.

Before public distribution, review [NOTICE](../NOTICE), including ROM
provenance, bundled dependencies and Microsoft runtime redistribution terms.
Check current security fixes: updates may postdate the pinned Qt SDK. Successful
packaging does not establish legal clearance or security certification.

Windows packages are unsigned. macOS has no Developer ID signature/notarization;
an ad-hoc signature does not identify the publisher. Public signed releases need
separately configured certificates, credentials and explicit approval.
