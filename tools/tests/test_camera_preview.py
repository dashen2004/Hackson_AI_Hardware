"""Exercise the LAN protocol with a fake board; no hardware or serial access."""
import importlib.util
import io
import json
import pathlib
import struct
import threading
import time
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from PIL import Image

SOURCE = pathlib.Path(__file__).resolve().parents[1] / "camera_preview.py"
spec = importlib.util.spec_from_file_location("camera_preview", SOURCE)
preview = importlib.util.module_from_spec(spec)
spec.loader.exec_module(preview)


class VideoProtocolTest(unittest.TestCase):
    def test_real_frames_pose_timestamp_and_board_restart(self):
        for width, height in ((240, 320), (120, 160)):
            with self.subTest(resolution=(width, height)):
                self.exercise_protocol(width, height)

    def exercise_protocol(self, width, height):
        board = {"frame": 42, "uptime": 8000, "completed": 6000}

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *_args):
                pass

            def do_GET(self):
                if self.path == "/frame":
                    payload = struct.pack("<6I", 0x314D4143, board["frame"], 7000, width, height, width * height * 2)
                    payload += bytes((128, 82, 128, 82)) * (width * height // 2)
                else:
                    payload = json.dumps(dict(uptime_ms=board["uptime"], valid=100,
                                              invalid=2, pose=dict(frame_id=board["frame"],
                                              completed_ms=board["completed"], processing_ms=3000,
                                              status="unknown", reason="none"))).encode()
                self.send_response(200)
                self.send_header("Content-Length", str(len(payload)))
                self.end_headers()
                self.wfile.write(payload)

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        camera = preview.Camera("unused")
        camera.update(endpoint=f"http://127.0.0.1:{server.server_port}")
        worker = threading.Thread(target=camera.network_run, daemon=True)
        worker.start()

        def wait_for(predicate):
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                with camera.lock:
                    if predicate(camera.state):
                        return
                time.sleep(0.02)
            self.fail("Timed out waiting for LAN frame")

        try:
            wait_for(lambda s: s["sequence"] == 1 and s["pose"] is not None)
            with camera.lock:
                png, pose_at = camera.png, camera.state["pose_at"]
            image = Image.open(io.BytesIO(png))
            self.assertEqual(image.size, (width, height))
            self.assertEqual(image.getpixel((width // 2, height // 2)), (77, 77, 77))
            self.assertLess(abs(time.time() - pose_at - 2), 1)
            time.sleep(0.7)
            with camera.lock:
                self.assertEqual(camera.state["sequence"], 1, "Repeated frame is not a new video frame")
                self.assertEqual(camera.state["pose_at"], pose_at, "Polling must not refresh old judgment age")
            board.update(frame=43)
            wait_for(lambda s: s["sequence"] == 2)
            board.update(frame=1, uptime=1000, completed=900)
            wait_for(lambda s: s["restarts"] == 1 and s["frame_id"] == 1)
        finally:
            camera.stop.set()
            worker.join(4)
            server.shutdown()
            server.server_close()


if __name__ == "__main__":
    unittest.main()
