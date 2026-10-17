"""Collect exact upstream Qt sources and license notices for bundled runtimes."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import tarfile
from urllib.request import urlopen


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--sources", type=Path, required=True)
    parser.add_argument("--notices", type=Path, required=True)
    args = parser.parse_args()
    if not all(part.isdigit() for part in args.version.split(".")) or len(args.version.split(".")) != 3:
        parser.error("expected a numeric Qt major.minor.patch version")
    args.sources.mkdir(parents=True, exist_ok=True)
    args.notices.mkdir(parents=True, exist_ok=True)
    series = ".".join(args.version.split(".")[:2])
    records = []
    for module in ("qtbase", "qtsvg", "qtimageformats"):
        filename = f"{module}-everywhere-src-{args.version}.tar.xz"
        url = f"https://download.qt.io/archive/qt/{series}/{args.version}/submodules/{filename}"
        archive = args.sources / filename
        if not archive.exists():
            with urlopen(url, timeout=120) as response, archive.open("wb") as output:
                while chunk := response.read(1024 * 1024):
                    output.write(chunk)
        with archive.open("rb") as original:
            digest = hashlib.file_digest(original, "sha256").hexdigest()
        # Compare with the publisher's digest, not just a locally computed hash.
        with urlopen(url + ".sha256", timeout=120) as response:
            expected = response.read().decode("ascii").split()[0].lower()
        if digest != expected:
            raise RuntimeError(f"upstream SHA256 mismatch: {filename}")
        count = 0
        with tarfile.open(archive, "r:xz") as source:
            for member in source:
                path = PurePosixPath(member.name)
                if not member.isfile() or path.is_absolute() or ".." in path.parts:
                    continue
                name = path.name.lower()
                if "LICENSES" not in path.parts and not name.startswith(
                    ("license", "licence", "copying", "copyright", "notice", "attribution")
                ):
                    continue
                target = args.notices / module / Path(*path.parts[1:])
                target.parent.mkdir(parents=True, exist_ok=True)
                with source.extractfile(member) as original, target.open("wb") as output:
                    output.write(original.read())
                count += 1
        if not count:
            raise RuntimeError(f"no Qt license notices found in {filename}")
        records.append({"module": module, "version": args.version, "url": url,
                        "sha256": digest, "archive": filename, "license_files": count})
        print(f"Verified {filename}; collected {count} license files", flush=True)
    manifest = json.dumps(records, indent=2) + "\n"
    (args.notices / "SOURCES.json").write_text(manifest, encoding="utf-8")
    (args.sources / "Qt-SOURCES.json").write_text(manifest, encoding="utf-8")


if __name__ == "__main__":
    main()
