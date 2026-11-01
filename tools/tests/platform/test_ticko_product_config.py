"""产品身份与私有配置检查；凭证均为合成测试数据，不连接云平台。"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/platform"))
from configure_ticko_product import LOCAL_HEADER, configure, render_header, validate_product


class ProductConfigTests(unittest.TestCase):
    def model(self):
        return {"product": {"productKey": "11878", "productSecret": "A" * 32}}

    def test_product_and_secret_format_are_checked(self):
        self.assertEqual(validate_product(self.model()), "A" * 32)
        for data in [{}, {"product": {}}, {"product": {"productKey": "11682", "productSecret": "A" * 32}},
                     {"product": {"productKey": "11878", "productSecret": "A" * 31}},
                     {"product": {"productKey": "11878", "productSecret": " " + "A" * 31}}]:
            with self.subTest(valid_shape=isinstance(data, dict)):
                with self.assertRaises(ValueError):
                    validate_product(data)

    def test_ignored_private_file_is_idempotent_and_never_silently_replaced(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            firmware = directory / "firmware"
            (firmware / LOCAL_HEADER.parent).mkdir(parents=True)
            subprocess.run(["git", "init", "--quiet", str(firmware)], check=True)
            model_path = directory / "model.json"
            model_path.write_text(json.dumps(self.model()))
            with self.assertRaises(ValueError):
                configure(model_path, firmware)
            self.assertFalse((firmware / LOCAL_HEADER).exists())
            (firmware / ".gitignore").write_text("/" + str(LOCAL_HEADER) + "\n")
            first = configure(model_path, firmware)
            self.assertEqual(first.read_text(), render_header("A" * 32))
            self.assertEqual(configure(model_path, firmware), first)
            if os.name != "nt":
                self.assertEqual(first.stat().st_mode & 0o777, 0o600)
            other = self.model()
            other["product"]["productSecret"] = "B" * 32
            model_path.write_text(json.dumps(other))
            with self.assertRaises(ValueError):
                configure(model_path, firmware)
            self.assertEqual(first.read_text(), render_header("A" * 32))

    def compile_header(self, *, local_pid=11878, secret="A" * 32, compiler="cc", language="c", create_local=True):
        executable = shutil.which(compiler)
        if executable is None:
            self.skipTest(f"缺少{compiler}")
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            source_header = ROOT / "src/firmware/apps/main/cloud/ljyd_cloud.h"
            shutil.copyfile(source_header, directory / "ljyd_cloud.h")
            (directory / "qm_spec_api.h").write_text(
                "typedef struct { int unused; } qm_spec_property_t;\n"
                "typedef struct { int unused; } qm_spec_property_operation_t;\n"
            )
            if create_local:
                body = render_header(secret).replace("LJYD_LOCAL_PRODUCT_PID 11878", f"LJYD_LOCAL_PRODUCT_PID {local_pid}")
                (directory / "ljyd_product_local.h").write_text(body)
            source = directory / "probe.c"
            source.write_text('#include "ljyd_cloud.h"\n#if CONFIG_XJIANG_CLOUD_PID != 11878\n#error Unexpected PID\n#endif\nint main(void) { return 0; }\n')
            standard = "c++17" if language == "c++" else "c11"
            return subprocess.run([executable, "-x", language, "-std=" + standard, "-fsyntax-only", str(source)],
                                  capture_output=True, text=True).returncode

    def test_real_public_header_accepts_matching_identity_in_c_and_cpp(self):
        self.assertEqual(self.compile_header(), 0)
        self.assertEqual(self.compile_header(compiler="c++", language="c++"), 0)

    def test_public_header_rejects_mismatched_pid(self):
        self.assertNotEqual(self.compile_header(local_pid=11682), 0)

    def test_public_header_rejects_missing_or_wrong_length_secret(self):
        self.assertNotEqual(self.compile_header(create_local=False), 0)
        self.assertNotEqual(self.compile_header(secret="A" * 31), 0)


if __name__ == "__main__":
    unittest.main()
