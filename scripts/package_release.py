"""Create the SD-root release archive for a compiled Touch Cursor overlay."""

import argparse
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--overlay", type=Path, required=True)
    parser.add_argument("--install-guide", type=Path, required=True)
    parser.add_argument("--license", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if not args.overlay.is_file() or args.overlay.stat().st_size == 0:
        parser.error("compiled overlay is missing or empty")
    if not args.install_guide.is_file():
        parser.error("installation guide is missing")
    if not args.license.is_file():
        parser.error("license is missing")
    args.output.parent.mkdir(parents=True, exist_ok=True)

    with ZipFile(args.output, "w", ZIP_DEFLATED, compresslevel=9) as archive:
        archive.write(args.overlay, "switch/.overlays/touch_cursor.ovl")
        archive.write(args.install_guide, "INSTALL.md")
        archive.write(args.license, "LICENSE")


if __name__ == "__main__":
    main()
