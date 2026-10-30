"""Local browser preview for the BF3901 serial camera firmware (pyserial only)."""

import argparse
import base64
import json
import pathlib
import re
import struct
import threading
import time
import zlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import serial

ROOT = pathlib.Path(__file__).resolve().parent


def rgb332_png(width, height, rows):
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for pixel in rows[y]:
            raw.extend(((pixel >> 5) * 255 // 7,
                        ((pixel >> 2) & 7) * 255 // 7,
                        (pixel & 3) * 255 // 3))

    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data
                + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF))

    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


class Camera:
    def __init__(self, port):
        self.port = port
        self.lock = threading.Lock()
        self.stop = threading.Event()
        self.png = b""
        self.state = dict(connected=False, port=port, enabled=True, error="",
                          sequence=0, frame_id=0, frame_at=0, frame_interval=0,
                          pose=None, pose_at=0, valid=0, invalid=0, restarts=0)

    def update(self, **values):
        with self.lock:
            self.state.update(values)

    def run(self):
        while not self.stop.is_set():
            try:
                with serial.Serial(port=None, baudrate=115200, timeout=0.2) as port:
                    port.dtr = False
                    port.rts = False
                    port.port = self.port
                    port.open()
                    port.reset_input_buffer()
                    self.update(connected=True, error="")
                    pending = False
                    requested_at = next_request = 0
                    frame = width = height = 0
                    rows = {}
                    while not self.stop.is_set():
                        now = time.monotonic()
                        with self.lock:
                            enabled = self.state["enabled"]
                        if enabled and now >= next_request and (not pending or now - requested_at > 8):
                            port.write(b"\r\ncamera_preview\r\n")
                            pending, requested_at = True, now
                        line = port.readline(4096).decode("ascii", errors="replace").strip()
                        start = re.search(r"CAMERA_PREVIEW (\d+) (\d+) (\d+)", line)
                        row = re.search(r"CAMERA_PIXEL (\d+) (\d+) ([A-Za-z0-9+/=]+)", line)
                        end = re.search(r"CAMERA_PREVIEW_END (\d+)", line)
                        pose = re.search(r"camera pose frame=(\d+) captured_ms=(\d+) status=(\w+) reason=(\w+) processing_ms=(\d+)", line)
                        counts = re.search(r"got (\d+) frames good=(\d+) empty=(\d+)", line)
                        if start:
                            frame, width, height = map(int, start.groups())
                            rows = {}
                            if not (1 <= width <= 160 and 1 <= height <= 160):
                                frame = width = height = 0
                        elif row and width and int(row[1]) == frame:
                            try:
                                pixels = base64.b64decode(row[3], validate=True)
                                if int(row[2]) < height and len(pixels) == width:
                                    rows[int(row[2])] = pixels
                            except ValueError:
                                pass
                        elif end:
                            if int(end[1]) == frame and width and len(rows) == height:
                                png = rgb332_png(width, height, rows)
                                received_at = time.time()
                                with self.lock:
                                    previous = self.state["frame_at"]
                                    self.png = png
                                    self.state.update(sequence=self.state["sequence"] + 1,
                                                      frame_id=frame, frame_at=received_at,
                                                      frame_interval=received_at - previous if previous else 0,
                                                      width=width, height=height, error="")
                            else:
                                self.update(error="上一张预览传输不完整，正在重试")
                            pending, next_request = False, time.monotonic() + 0.15
                        elif pose:
                            self.update(pose=dict(frame_id=int(pose[1]), captured_ms=int(pose[2]),
                                                  status=pose[3], reason=pose[4], processing_ms=int(pose[5])),
                                        pose_at=time.time())
                        elif counts:
                            self.update(valid=int(counts[2]), invalid=int(counts[3]))
                        elif "camera pose frame=" in line and "error=" in line:
                            self.update(pose=dict(status="error", reason="inference_error"), pose_at=time.time())
                        elif "Loaded app from partition" in line:
                            with self.lock:
                                self.state["restarts"] += 1
                                self.state.update(pose=None, pose_at=0, frame_at=0)
                            pending, next_request = False, time.monotonic() + 3
                        elif "Guru Meditation" in line or "assert failed" in line:
                            self.update(error="设备出现异常，等待重新连接")
            except (serial.SerialException, OSError) as exc:
                self.update(connected=False, error=f"串口连接中断：{exc}")
                self.stop.wait(2)
        self.update(connected=False)


def handler_for(camera):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            pass

        def send(self, content, mime, status=200):
            self.send_response(status)
            self.send_header("Content-Type", mime)
            self.send_header("Content-Length", str(len(content)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(content)

        def do_GET(self):
            route = self.path.split("?", 1)[0]
            if route == "/":
                self.send((ROOT / "camera_preview.html").read_bytes(), "text/html; charset=utf-8")
            elif route == "/api/state":
                with camera.lock:
                    state = dict(camera.state, now=time.time())
                self.send(json.dumps(state, ensure_ascii=False).encode(), "application/json; charset=utf-8")
            elif route == "/api/frame.png":
                with camera.lock:
                    png = camera.png
                self.send(png, "image/png", 200 if png else 503)
            else:
                self.send(b"Not found", "text/plain", 404)

        def do_POST(self):
            if self.path == "/api/pause":
                camera.update(enabled=False)
            elif self.path == "/api/resume":
                camera.update(enabled=True)
            else:
                self.send(b"Not found", "text/plain", 404)
                return
            self.send(b"{}", "application/json")

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="COM7")
    parser.add_argument("--http-port", default=8765, type=int)
    args = parser.parse_args()
    camera = Camera(args.port)
    server = ThreadingHTTPServer(("127.0.0.1", args.http_port), handler_for(camera))
    worker = threading.Thread(target=camera.run, daemon=True)
    worker.start()
    print(f"Camera preview: http://127.0.0.1:{args.http_port}/", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        camera.stop.set()
        server.server_close()
        worker.join(timeout=3)


if __name__ == "__main__":
    main()
