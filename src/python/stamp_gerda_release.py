#!/usr/bin/env python3
"""Stamp a unified firmware.bin with a targets.json hardware path, keeping baked options."""
import json
import shutil
import sys
from pathlib import Path

from UnifiedConfiguration import appendToFirmware, findFirmwareEnd


def read_baked_options(firmware_path):
    with open(firmware_path, "rb") as f:
        end = findFirmwareEnd(f)
        f.seek(end + 128 + 16)
        raw = f.read(512).split(b"\0", 1)[0]
    return json.loads(raw.decode("utf-8"))


def stamp(src, dst, target_path, options_json):
    shutil.copy2(src, dst)
    targets = json.loads(Path("hardware/targets.json").read_text())
    keys = ".".join(f'"{p}"' for p in target_path.split("."))
    # local jmespath-free walk
    node = targets
    for part in target_path.split("."):
        node = node[part]
    config = node
    moduletype = "tx" if ".tx_" in target_path else "rx"
    layout = f"hardware/{'TX' if moduletype == 'tx' else 'RX'}/{config['layout_file']}"
    with open(dst, "r+b") as firmware_file:
        appendToFirmware(
            firmware_file,
            config["product_name"],
            config["lua_name"],
            options_json,
            config,
            layout,
            None,
        )


def main():
    if len(sys.argv) != 4:
        sys.stderr.write("usage: stamp_gerda_release.py <src.bin> <dst.bin> <targets.json.path>\n")
        return 2
    src, dst, target = sys.argv[1], sys.argv[2], sys.argv[3]
    options = json.dumps(read_baked_options(src), separators=(",", ":"))
    stamp(src, dst, target, options)
    print(f"stamped {dst} as {target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
