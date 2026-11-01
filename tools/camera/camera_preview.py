"""Local browser preview for the BF3901 serial camera firmware (pyserial only)."""

import argparse
import base64
import json
import io
import http.client
import pathlib
import re
import struct
import threading
import time
import zlib
import urllib.parse
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
        self.changed = threading.Condition(self.lock)
        self.stop = threading.Event()
        self.png = b""
        self.jpeg = b""
        self.state = dict(connected=False, port=port, enabled=True, error="",
                          sequence=0, frame_id=0, frame_at=0, frame_interval=0,
                          pose=None, pose_at=0, valid=0, invalid=0, restarts=0,
                          transport="serial", endpoint="", network_error="")

    def network_run(self):
        # Optional libraries are needed only for full-resolution LAN video.
        try:
            import numpy as np
            from PIL import Image
        except ImportError:
            self.update(network_error="局域网视频需要安装 numpy 和 Pillow")
            return
        frame_connection = status_connection = None
        active_endpoint = ""
        last_id = None
        last_uptime = None
        last_pose = None
        next_status = 0
        while not self.stop.is_set():
            with self.lock:
                endpoint, enabled = self.state["endpoint"], self.state["enabled"]
            if not endpoint:
                self.stop.wait(0.5)
                continue
            try:
                if active_endpoint != endpoint or frame_connection is None:
                    for connection in (frame_connection, status_connection):
                        if connection:
                            connection.close()
                    parsed = urllib.parse.urlsplit(endpoint)
                    frame_connection = http.client.HTTPConnection(parsed.hostname, parsed.port, timeout=2)
                    status_connection = http.client.HTTPConnection(parsed.hostname, parsed.port, timeout=2)
                    active_endpoint, last_id, last_pose, last_uptime = endpoint, None, None, None
                started = time.monotonic()
                if started >= next_status:
                    status_connection.request("GET", "/status")
                    response = status_connection.getresponse()
                    payload = response.read(4096)
                    if response.status != 200:
                        raise RuntimeError(f"检测接口 HTTP {response.status}")
                    status = json.loads(payload)
                    uptime = status["uptime_ms"]
                    if last_uptime is not None and uptime < last_uptime:
                        with self.lock:
                            self.state["restarts"] += 1
                            self.state.update(frame_at=0, pose=None, pose_at=0)
                        last_id = last_pose = None
                    last_uptime = uptime
                    pose = status.get("pose")
                    self.update(transport="wifi", valid=status["valid"], invalid=status["invalid"], network_error="",
                                error="摄像头尚未收到有效图像，请检查排线和数据线" if status["valid"] == 0 and status["invalid"] > 30 else "")
                    if pose and (pose["frame_id"], pose["completed_ms"]) != last_pose:
                        last_pose = pose["frame_id"], pose["completed_ms"]
                        self.update(pose=pose, pose_at=time.time() - max(0, uptime - pose["completed_ms"]) / 1000)
                    next_status = time.monotonic() + 0.25
                if enabled:
                    frame_connection.request("GET", "/frame")
                    response = frame_connection.getresponse()
                    payload = response.read(240 * 320 * 2 + 25)
                    if response.status == 503:
                        self.stop.wait(0.1)
                        continue
                    if response.status != 200 or len(payload) < 24:
                        raise RuntimeError("画面接口返回不完整")
                    magic, frame_id, captured, width, height, size = struct.unpack("<6I", payload[:24])
                    if (magic != 0x314D4143 or (width, height) not in ((240, 320), (120, 160))
                            or size != width * height * 2 or len(payload) != size + 24):
                        raise RuntimeError("图像协议校验失败")
                    if last_id != frame_id:
                        pairs = np.frombuffer(payload, dtype=np.uint8, offset=24).reshape(height, width // 2, 4).astype(np.int32)
                        y = pairs[:, :, [1, 3]].reshape(height, width) - 16
                        u = np.repeat(pairs[:, :, 0], 2, axis=1) - 128
                        v = np.repeat(pairs[:, :, 2], 2, axis=1) - 128
                        rgb = np.stack(((298*y + 409*v + 128) >> 8,
                                        (298*y - 100*u - 208*v + 128) >> 8,
                                        (298*y + 516*u + 128) >> 8), axis=-1)
                        image = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8))
                        output = io.BytesIO()
                        image.save(output, format="JPEG", quality=85)
                        jpeg = output.getvalue()
                        output = io.BytesIO()
                        image.save(output, format="PNG")
                        received_at = time.time()
                        with self.changed:
                            previous = self.state["frame_at"]
                            self.png, self.jpeg = output.getvalue(), jpeg
                            self.state.update(sequence=self.state["sequence"] + 1, frame_id=frame_id,
                                              frame_at=received_at, captured_ms=captured,
                                              frame_interval=received_at - previous if previous else 0,
                                              width=width, height=height, transport="wifi", error="")
                            self.changed.notify_all()
                        last_id = frame_id
                self.stop.wait(max(0.01, 0.1 - (time.monotonic() - started)))
            except (OSError, ValueError, KeyError, RuntimeError, http.client.HTTPException) as exc:
                self.update(network_error=f"等待局域网视频：{exc}", transport="serial")
                for connection in (frame_connection, status_connection):
                    if connection:
                        connection.close()
                frame_connection = status_connection = None
                self.stop.wait(1)

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
                    requested_at = next_request = next_endpoint = 0
                    frame = width = height = 0
                    rows = {}
                    while not self.stop.is_set():
                        now = time.monotonic()
                        with self.lock:
                            enabled = self.state["enabled"]
                            wifi = self.state["transport"] == "wifi"
                        if now >= next_endpoint:
                            port.write(b"\r\ncamera_endpoint\r\n")
                            next_endpoint = now + 5
                        if enabled and not wifi and now >= next_request and (not pending or now - requested_at > 8):
                            port.write(b"\r\ncamera_preview\r\n")
                            pending, requested_at = True, now
                        line = port.readline(4096).decode("ascii", errors="replace").strip()
                        start = re.search(r"CAMERA_PREVIEW (\d+) (\d+) (\d+)", line)
                        row = re.search(r"CAMERA_PIXEL (\d+) (\d+) ([A-Za-z0-9+/=]+)", line)
                        end = re.search(r"CAMERA_PREVIEW_END (\d+)", line)
                        pose = re.search(r"camera pose frame=(\d+) captured_ms=(\d+) status=(\w+) reason=(\w+) processing_ms=(\d+)", line)
                        counts = re.search(r"got (\d+) frames good=(\d+) empty=(\d+)", line)
                        endpoint_match = re.search(r"CAMERA_ENDPOINT (http://[0-9.]+:8080|offline)", line)
                        if endpoint_match:
                            if endpoint_match[1] == "offline":
                                self.update(endpoint="", transport="serial", network_error="开发板尚未取得 Wi-Fi 地址")
                            else:
                                self.update(endpoint=endpoint_match[1])
                        elif start:
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
                            if not wifi and int(end[1]) == frame and width and len(rows) == height:
                                png = rgb332_png(width, height, rows)
                                received_at = time.time()
                                with self.lock:
                                    previous = self.state["frame_at"]
                                    self.png = png
                                    self.state.update(sequence=self.state["sequence"] + 1,
                                                      frame_id=frame, frame_at=received_at,
                                                      frame_interval=received_at - previous if previous else 0,
                                                      width=width, height=height, error="")
                            elif not wifi:
                                self.update(error="上一张预览传输不完整，正在重试")
                            pending, next_request = False, time.monotonic() + 0.15
                        elif pose and not wifi:
                            self.update(pose=dict(frame_id=int(pose[1]), captured_ms=int(pose[2]),
                                                  status=pose[3], reason=pose[4], processing_ms=int(pose[5])),
                                        pose_at=time.time())
                        elif counts:
                            self.update(valid=int(counts[2]), invalid=int(counts[3]))
                        elif "camera pose frame=" in line and "error=" in line and not wifi:
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
            elif route == "/api/stream.mjpg":
                self.send_response(200)
                self.send_header("Content-Type", "multipart/x-mixed-replace; boundary=camera-frame")
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                sequence = -1
                try:
                    while not camera.stop.is_set():
                        with camera.changed:
                            camera.changed.wait_for(lambda: camera.state["sequence"] != sequence or camera.stop.is_set(), timeout=2)
                            jpeg, current = camera.jpeg, camera.state["sequence"]
                            if not jpeg or current == sequence:
                                sequence = current
                                continue
                            sequence = current
                        self.wfile.write(b"--camera-frame\r\nContent-Type: image/jpeg\r\nContent-Length: "
                                         + str(len(jpeg)).encode() + b"\r\n\r\n" + jpeg + b"\r\n")
                        self.wfile.flush()
                except (BrokenPipeError, ConnectionResetError, OSError):
                    pass
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
    network_worker = threading.Thread(target=camera.network_run, daemon=True)
    worker.start()
    network_worker.start()
    print(f"Camera preview: http://127.0.0.1:{args.http_port}/", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        camera.stop.set()
        server.server_close()
        worker.join(timeout=3)
        network_worker.join(timeout=3)


if __name__ == "__main__":
    main()
