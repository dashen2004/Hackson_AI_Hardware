#!/usr/bin/env python3
"""从项目外的原始物模型配置Ticko本机凭证，不输出产品密钥。"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
TARGET_PID = 11878
LOCAL_HEADER = Path("apps/main/cloud/ljyd_product_local.h")


def validate_product(data):
    if not isinstance(data, dict) or not isinstance(data.get("product"), dict):
        raise ValueError("缺少产品对象")
    product = data["product"]
    if str(product.get("productKey")) != str(TARGET_PID):
        raise ValueError("物模型产品ID与Ticko目标11878不一致")
    secret = product.get("productSecret")
    if not isinstance(secret, str) or re.fullmatch(r"[0-9A-Fa-f]{32}", secret) is None:
        raise ValueError("需使用含有效32位产品密钥的原始导出，脱敏文件不能用于配置")
    return secret


def render_header(secret):
    return (
        "#ifndef LJYD_PRODUCT_LOCAL_H\n#define LJYD_PRODUCT_LOCAL_H\n\n"
        "/* Local credential for PID 11878; ignored by Git. */\n"
        "#define LJYD_LOCAL_PRODUCT_PID 11878\n"
        f'#define CONFIG_XJIANG_CLOUD_SECERT "{secret}"\n\n'
        "#endif\n"
    )


def configure(model_path, firmware_dir=None):
    firmware_dir = Path(firmware_dir) if firmware_dir else ROOT / "src/firmware"
    data = json.loads(Path(model_path).read_text(encoding="utf-8"))
    secret = validate_product(data)
    target = firmware_dir / LOCAL_HEADER
    ignored = subprocess.run(
        ["git", "check-ignore", "--quiet", "--", str(LOCAL_HEADER)],
        cwd=firmware_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    )
    if ignored.returncode != 0:
        raise ValueError("本机凭证路径尚未被Git忽略，未写入")
    content = render_header(secret)
    if target.exists():
        if target.read_text(encoding="utf-8") != content:
            raise ValueError("已有不同本机配置，未覆盖；请先核对对应产品与密钥")
        if os.name != "nt":
            target.chmod(0o600)
        return target
    fd = os.open(target, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as stream:
        stream.write(content)
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", type=Path, required=True,
                        help="项目外的原始物模型JSON，需含11878对应密钥")
    args = parser.parse_args()
    try:
        configure(args.model)
    except (ValueError, OSError, TypeError):
        parser.exit(1, "配置失败：请检查原始JSON、PID11878、密钥格式与已有本机文件；未输出凭证。\n")
    print("已配置PID11878；密钥仅存本机Git忽略头文件，未联网验证。")


if __name__ == "__main__":
    main()
