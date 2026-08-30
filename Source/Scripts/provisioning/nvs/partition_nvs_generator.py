#!/usr/bin/env python3
"""
@file       partition_nvs_generator.py
@brief      Generic NVS partition generator supporting arbitrary templates and variable substitution.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import glob
import os
import secrets
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, Any, List, Optional

SCRIPT_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPT_DIR))

from factory.partition_parser import PartitionTableParser


def generate_nvs_keys(key_out_path: Path) -> bytes:
    """Generates a 64-byte AES-XTS key binary for ESP32 NVS encryption."""
    key_bytes = secrets.token_bytes(64)
    key_out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(key_out_path, "wb") as f:
        f.write(key_bytes)
    return key_bytes


def render_template_csv(template_path: Path, output_csv_path: Path, variables: Dict[str, Any], project_root: Path) -> None:
    """Renders a template CSV by replacing all {KEY} placeholders with provided variables."""
    if not template_path.exists():
        raise FileNotFoundError(f"Template CSV missing: {template_path}")

    with open(template_path, "r", encoding="utf-8") as f:
        content = f.read()

    for key, value in variables.items():
        placeholder = f"{{{key}}}"
        if isinstance(value, str) and (value.endswith(".crt") or value.endswith(".pem") or value.endswith(".bin") or value.endswith(".key")):
            val_path = Path(value).resolve() if Path(value).is_absolute() else (project_root / value).resolve()
            if val_path.exists():
                value = str(val_path)

        content = content.replace(placeholder, str(value))

    output_csv_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_csv_path, "w", encoding="utf-8") as f:
        f.write(content)


def find_nvs_partition_gen_tool() -> Optional[List[str]]:
    """Locates nvs_partition_gen across pip modules, IDF_PATH, and standard installation directories."""
    # 1. Check if esp_idf_nvs_partition_gen or nvs_partition_gen is installed in Python
    try:
        import esp_idf_nvs_partition_gen
        return [sys.executable, "-m", "esp_idf_nvs_partition_gen.nvs_partition_gen"]
    except ImportError:
        pass

    try:
        import nvs_partition_gen
        return [sys.executable, "-m", "nvs_partition_gen"]
    except ImportError:
        pass

    # 2. Check IDF_PATH environment variable
    idf_path = os.environ.get("IDF_PATH")
    if idf_path:
        candidate = Path(idf_path) / "components" / "nvs_flash" / "nvs_partition_generator" / "nvs_partition_gen.py"
        if candidate.exists():
            return [sys.executable, str(candidate.resolve())]

    # 3. Check system PATH
    which_tool = shutil.which("nvs_partition_gen.py") or shutil.which("nvs_partition_gen")
    if which_tool:
        return [which_tool] if which_tool.endswith(".exe") else [sys.executable, which_tool]

    # 4. Check common Windows Espressif installation paths
    common_patterns = [
        r"C:\Espressif\frameworks\esp-idf*\components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py",
        r"C:\esp\esp-idf*\components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py",
        os.path.expanduser(r"~\esp\esp-idf\components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py"),
        os.path.expanduser(r"~/.espressif/frameworks/esp-idf*/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py")
    ]
    for pattern in common_patterns:
        matches = glob.glob(pattern)
        if matches:
            return [sys.executable, str(Path(matches[-1]).resolve())]

    return None


def invoke_nvs_partition_gen(csv_path: Path, bin_out_path: Path, partition_size: int, key_file_path: Path) -> bool:
    """Executes nvs_partition_gen utility to generate encrypted binary."""
    bin_out_path.parent.mkdir(parents=True, exist_ok=True)
    tool_cmd = find_nvs_partition_gen_tool()

    if not tool_cmd:
        print("[ERROR] Could not find 'nvs_partition_gen'. Please run: pip install esp-idf-nvs-partition-gen", file=sys.stderr)
        return False

    # If the key file already exists, pass it via --inputkey; otherwise use --keygen --keyfile
    key_args = ["--inputkey", str(key_file_path.resolve())] if key_file_path.exists() else ["--keygen", "--keyfile", str(key_file_path.resolve())]

    cmd = tool_cmd + [
        "encrypt",
        str(csv_path.resolve()),
        str(bin_out_path.resolve()),
        hex(partition_size),
    ] + key_args

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0:
            print(f"[ERROR] nvs_partition_gen execution failed:\n{res.stderr}", file=sys.stderr)
            return False
        return True
    except Exception as e:
        print(f"[ERROR] Exception during nvs_partition_gen execution: {e}", file=sys.stderr)
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Generic NVS Partition Generator.")
    parser.add_argument("--template", type=Path, required=True, help="Path to input template CSV")
    parser.add_argument("--out-csv", type=Path, required=True, help="Path to write rendered CSV")
    parser.add_argument("--out-bin", type=Path, required=True, help="Path to write encrypted NVS binary")
    parser.add_argument("--out-key", type=Path, required=True, help="Path to NVS encryption key binary")
    parser.add_argument("--partitions-csv", type=Path, required=False, help="Path to partitions.csv")
    parser.add_argument("--target-partition", type=str, required=False, help="Target partition name in partitions.csv")
    parser.add_argument("--size", type=lambda x: int(x, 0), required=False, help="Explicit partition size in bytes")
    args = parser.parse_args()

    size: int
    if args.size:
        size = args.size
    elif args.partitions_csv and args.target_partition:
        pt_parser = PartitionTableParser(args.partitions_csv)
        size = pt_parser.get_size(args.target_partition)
    else:
        print("[ERROR] Provide either --size or both --partitions-csv and --target-partition", file=sys.stderr)
        return 1

    if not args.out_key.exists():
        generate_nvs_keys(args.out_key)

    if invoke_nvs_partition_gen(args.out_csv, args.out_bin, size, args.out_key) and args.out_bin.exists():
        print(f"[SUCCESS] Encrypted NVS partition generated: {args.out_bin}")
        return 0

    return 1


if __name__ == "__main__":
    sys.exit(main())