"""Request one diagnostic thumbnail from camera_snapshot; save a local PNG.

Requires pyserial (included in the project's ESP-IDF Python environment).
Close other serial monitors first. This command does not reset the board.
"""

import argparse
import pathlib
import re
import struct
import time
import zlib

import serial


def png_chunk(kind, data):
    return (struct.pack(">I", len(data)) + kind + data
            + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF))


def save_png(path, width, height, rows):
    pixels = b"".join(b"\0" + rows[y] for y in range(height))
    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + png_chunk(b"IHDR", header)
                     + png_chunk(b"IDAT", zlib.compress(pixels))
                     + png_chunk(b"IEND", b""))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="COM7")
    parser.add_argument("--output", required=True, type=pathlib.Path,
                        help="PNG destination; keep personal photos outside the repo")
    parser.add_argument("--timeout", default=40.0, type=float)
    args = parser.parse_args()
    width = height = frame = 0
    rows = {}
    with serial.Serial(port=None, baudrate=115200, timeout=0.5) as port:
        port.dtr = False
        port.rts = False
        port.port = args.port
        port.open()
        port.reset_input_buffer()
        # A leading newline clears any unfinished console input.
        port.write(b"\r\ncamera_snapshot\r\n")
        deadline = time.monotonic() + args.timeout
        while time.monotonic() < deadline:
            line = port.readline(4096).decode("ascii", errors="replace").strip()
            start = re.search(r"CAMERA_RGB (\d+) (\d+) (\d+)", line)
            row = re.search(r"CAMERA_ROW (\d+) ([0-9a-f]+)", line)
            if start:
                width, height, frame = map(int, start.groups())
                if not (1 <= width <= 320 and 1 <= height <= 320):
                    raise RuntimeError("Invalid thumbnail dimensions")
                rows = {}
            elif row and width:
                index = int(row[1])
                if index < height and len(row[2]) == width * 6:
                    rows[index] = bytes.fromhex(row[2])
            elif "CAMERA_END" in line:
                if width and len(rows) == height:
                    save_png(args.output, width, height, rows)
                    print(f"Saved frame {frame}: {width}x{height} -> {args.output.resolve()}")
                    return
                raise RuntimeError("Incomplete thumbnail; retry the command")
        raise TimeoutError("No complete thumbnail; check firmware, CLI and serial ownership")


if __name__ == "__main__":
    main()
