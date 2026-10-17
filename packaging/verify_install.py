"""Check the installed payload, then start it without the Qt SDK environment."""
import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("prefix", type=Path)
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    if sys.platform == "darwin":
        bundle = prefix / "elektronika-mk61.app" / "Contents"
        executable = bundle / "MacOS" / "elektronika-mk61"
        notices = bundle / "Resources" / "licenses"
        required = [bundle / "Resources" / "elektronika-mk61.icns",
                    bundle / "Frameworks" / "QtCore.framework",
                    notices / "Qt" / "SOURCES.json"]
    else:
        executable = prefix / "bin" / ("elektronika-mk61.exe" if os.name == "nt" else "elektronika-mk61")
        notices = prefix / "share" / "doc" / "elektronika-mk61"
        required = []
        if os.name == "nt":
            required += [prefix / "bin" / "Qt6Core.dll", notices / "Qt" / "SOURCES.json"]
        else:
            required += [prefix / "share/icons/hicolor/scalable/apps/elektronika-mk61.svg"]
            desktop = prefix / "share/applications/elektronika-mk61.desktop"
            contents = desktop.read_text(encoding="utf-8")
            if "Exec=/usr/bin/elektronika-mk61\n" not in contents:
                raise RuntimeError("package launcher contains the wrong installation prefix")
    required += [executable, notices / "LICENSE", notices / "NOTICE", notices / "DejaVu-LICENSE.txt"]
    for item in required:
        if not item.exists():
            raise RuntimeError(f"missing installed file: {item}")
    forbidden = {"CMakeCache.txt", "CMakeFiles", "tests", "upstream"}
    # On Linux /usr is also the live clean-container prefix; do not scan all
    # distribution files looking for application-specific build artifacts.
    roots = [bundle] if sys.platform == "darwin" else [prefix / "bin", notices]
    for root in roots:
        for path in root.rglob("*"):
            if path.name in forbidden or path.name.startswith("pmk-") and path.is_file():
                raise RuntimeError(f"non-runtime file in package: {path}")
    env = os.environ.copy()
    for name in ("QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QML2_IMPORT_PATH",
                 "QML_IMPORT_PATH", "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH", "LD_LIBRARY_PATH"):
        env.pop(name, None)
    sdk = env.get("QT_ROOT_DIR", "")
    if sdk:
        normalized_sdk = os.path.normcase(os.path.normpath(sdk))
        env["PATH"] = os.pathsep.join(
            entry for entry in env.get("PATH", "").split(os.pathsep)
            if not os.path.normcase(os.path.normpath(entry)).startswith(normalized_sdk + os.sep)
            and os.path.normcase(os.path.normpath(entry)) != normalized_sdk)
    env["QT_QPA_PLATFORM"] = "offscreen"
    # Use a non-SDK working directory as well as a cleaned environment.
    subprocess.run([str(executable), "--smoke-test"], cwd=prefix, env=env,
                   check=True, timeout=30)
    print(f"Installed payload and startup verified: {prefix}")


if __name__ == "__main__":
    main()
