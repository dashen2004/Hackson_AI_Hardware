#!/usr/bin/env python3
"""
Batch convert PNG files in ./image/ to LVGL ARGB8888 .bin files in ./bin/.

Usage:
    python3 convert_to_bin.py
    python3 convert_to_bin.py icon_tomato.png icon_alarm.png
"""

import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

from LVGLImage import LVGLImage, ColorFormat, CompressMethod

SRC_DIR = SCRIPT_DIR / "image"
OUT_DIR = SCRIPT_DIR / "bin"


def convert_one(png_path: Path) -> None:
    out_path = OUT_DIR / f"{png_path.stem}.bin"
    img = LVGLImage().from_png(str(png_path), ColorFormat.ARGB8888)
    img.to_bin(str(out_path), compress=CompressMethod.NONE)
    print(f"OK  {png_path.name} -> bin/{out_path.name}")


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    if len(sys.argv) > 1:
        png_files = []
        for arg in sys.argv[1:]:
            path = Path(arg)
            if not path.is_absolute():
                path = SRC_DIR / path
            if not path.exists():
                print(f"FAIL missing: {path}")
                return 1
            png_files.append(path)
    else:
        png_files = sorted(SRC_DIR.glob("*.png"))
        if not png_files:
            print(f"No PNG files found in {SRC_DIR}")
            return 1

    ok = 0
    fail = 0
    for png in png_files:
        try:
            convert_one(png)
            ok += 1
        except Exception as exc:
            print(f"FAIL {png.name}: {exc}")
            fail += 1

    print(f"\nDone: {ok} converted, {fail} failed -> {OUT_DIR}")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
