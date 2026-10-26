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
import zlib
from pathlib import Path
from typing import Dict, Any, List, Optional

SCRIPT_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPT_DIR))

from factory.partition_parser import PartitionTableParser


def generate_nvs_keys(
    key_out_path: Path, key_size_bytes: int, key_bytes: Optional[bytes] = None
) -> bytes:
    """Write and return the configured-size NVS key, generating random bytes if needed."""
    if key_size_bytes <= 0:
        raise ValueError("NVS encryption key size must be greater than zero")
    if key_bytes is None:
        key_bytes = secrets.token_bytes(key_size_bytes)
    elif len(key_bytes) != key_size_bytes:
        raise ValueError("Configured NVS key does not match key_size_bytes")

    key_out_path.parent.mkdir(parents=True, exist_ok=True)
    key_out_path.write_bytes(key_bytes)
    return key_bytes


def ensure_nvs_keys(key_path: Path, key_size_bytes: int) -> bytes:
    """Reuse a valid generated key or create one for a new NVS artifact set."""
    if key_path.exists():
        key_bytes = key_path.read_bytes()
        if len(key_bytes) == key_size_bytes:
            return key_bytes

    return generate_nvs_keys(key_path, key_size_bytes)


def write_nvs_keys_partition(
    key_bytes: bytes, partition_image_path: Path, key_size_bytes: int
) -> None:
    """Create the NVS key partition header (keys, CRC, and erased alignment bytes)."""
    if len(key_bytes) != key_size_bytes:
        raise ValueError("NVS encryption key does not match configured key_size_bytes")

    crc = zlib.crc32(key_bytes)
    partition_image_path.parent.mkdir(parents=True, exist_ok=True)
    partition_image_path.write_bytes(
        key_bytes + crc.to_bytes(4, "little") + b"\xff" * 12
    )


def render_template_csv(
    template_path: Path,
    output_csv_path: Path,
    variables: Dict[str, Any],
    project_root: Path,
) -> None:
    """
    Renders a template CSV by substituting all {KEY} placeholders with provided variables.
    Cleanly strips optional file rows if their target path is empty or does not exist.
    """
    if not template_path.exists():
        raise FileNotFoundError(f"Template CSV missing: {template_path}")

    with open(template_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    rendered_lines = []

    for line in lines:
        stripped_line = line.strip()
        if not stripped_line or stripped_line.startswith("#"):
            rendered_lines.append(line)
            continue

        # Check if line contains any unsupplied or empty optional file placeholders
        skip_line = False
        for key, value in variables.items():
            placeholder = f"{{{key}}}"
            if placeholder in stripped_line:
                # Handle file paths
                if isinstance(value, str) and any(
                    value.endswith(ext) for ext in [".crt", ".pem", ".bin", ".key"]
                ):
                    val_path = (
                        Path(value).resolve()
                        if Path(value).is_absolute()
                        else (project_root / value).resolve()
                    )
                    if val_path.exists():
                        value = str(val_path)
                    else:
                        # If optional file doesn't exist on disk, omit this entry
                        skip_line = True
                        break
                elif value is None or str(value).strip() == "":
                    skip_line = True
                    break

                stripped_line = stripped_line.replace(placeholder, str(value))

        # Check if any unresolved {PLACEHOLDERS} remain
        if "{" in stripped_line and "}" in stripped_line:
            skip_line = True

        if not skip_line:
            rendered_lines.append(stripped_line + "\n")

    output_csv_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_csv_path, "w", encoding="utf-8") as f:
        f.writelines(rendered_lines)


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
        candidate = (
            Path(idf_path)
            / "components"
            / "nvs_flash"
            / "nvs_partition_generator"
            / "nvs_partition_gen.py"
        )
        if candidate.exists():
            return [sys.executable, str(candidate.resolve())]

    # 3. Check system PATH
    which_tool = shutil.which("nvs_partition_gen.py") or shutil.which(
        "nvs_partition_gen"
    )
    if which_tool:
        return (
            [which_tool]
            if which_tool.endswith(".exe")
            else [sys.executable, which_tool]
        )

    # 4. Check common Windows Espressif installation paths
    common_patterns = [
        r"C:\Espressif\frameworks\esp-idf*\components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py",
        r"C:\esp\esp-idf*\components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py",
        os.path.expanduser(
            r"~\esp\esp-idf\components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py"
        ),
        os.path.expanduser(
            r"~/.espressif/frameworks/esp-idf*/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py"
        ),
    ]
    for pattern in common_patterns:
        matches = glob.glob(pattern)
        if matches:
            return [sys.executable, str(Path(matches[-1]).resolve())]

    return None


def invoke_nvs_partition_gen(
    csv_path: Path,
    bin_out_path: Path,
    partition_size: int,
    key_file_path: Optional[Path] = None,
    encrypt: bool = False,
) -> bool:
    """Executes nvs_partition_gen utility to generate plaintext or encrypted binary."""
    bin_out_path.parent.mkdir(parents=True, exist_ok=True)
    tool_cmd = find_nvs_partition_gen_tool()

    if not tool_cmd:
        print(
            "[ERROR] Could not find 'nvs_partition_gen'. Please run: pip install esp-idf-nvs-partition-gen",
            file=sys.stderr,
        )
        return False

    if encrypt:
        if key_file_path is None:
            print(
                "[ERROR] Key file path required when encryption is enabled.",
                file=sys.stderr,
            )
            return False

        key_args = (
            ["--inputkey", str(key_file_path.resolve())]
            if key_file_path.exists()
            else ["--keygen", "--keyfile", str(key_file_path.resolve())]
        )

        cmd = (
            tool_cmd
            + [
                "encrypt",
                str(csv_path.resolve()),
                str(bin_out_path.resolve()),
                hex(partition_size),
            ]
            + key_args
        )
    else:
        cmd = tool_cmd + [
            "generate",
            str(csv_path.resolve()),
            str(bin_out_path.resolve()),
            hex(partition_size),
        ]

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0:
            print(
                f"[ERROR] nvs_partition_gen execution failed:\n{res.stderr}",
                file=sys.stderr,
            )
            return False
        return True
    except Exception as e:
        print(
            f"[ERROR] Exception during nvs_partition_gen execution: {e}",
            file=sys.stderr,
        )
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Generic NVS Partition Generator.")
    parser.add_argument(
        "--template", type=Path, required=True, help="Path to input template CSV"
    )
    parser.add_argument(
        "--out-csv", type=Path, required=True, help="Path to write rendered CSV"
    )
    parser.add_argument(
        "--out-bin", type=Path, required=True, help="Path to write NVS binary"
    )
    parser.add_argument(
        "--out-key",
        type=Path,
        required=False,
        default=None,
        help="Path to NVS encryption key binary",
    )
    parser.add_argument(
        "--key-size-bytes",
        type=int,
        required=False,
        help="Configured NVS encryption key size in bytes (required when generating a key)",
    )
    parser.add_argument(
        "--encrypt",
        action="store_true",
        default=False,
        help="Generate encrypted NVS partition",
    )
    parser.add_argument(
        "--partitions-csv", type=Path, required=False, help="Path to partitions.csv"
    )
    parser.add_argument(
        "--target-partition",
        type=str,
        required=False,
        help="Target partition name in partitions.csv",
    )
    parser.add_argument(
        "--size",
        type=lambda x: int(x, 0),
        required=False,
        help="Explicit partition size in bytes",
    )
    args = parser.parse_args()

    size: int
    if args.size:
        size = args.size
    elif args.partitions_csv and args.target_partition:
        pt_parser = PartitionTableParser(args.partitions_csv)
        size = pt_parser.get_size(args.target_partition)
    else:
        print(
            "[ERROR] Provide either --size or both --partitions-csv and --target-partition",
            file=sys.stderr,
        )
        return 1

    if args.encrypt and args.out_key and not args.out_key.exists():
        if args.key_size_bytes is None:
            print(
                "[ERROR] --key-size-bytes is required when generating an NVS key",
                file=sys.stderr,
            )
            return 1
        generate_nvs_keys(args.out_key, args.key_size_bytes)

    if (
        invoke_nvs_partition_gen(
            args.out_csv,
            args.out_bin,
            size,
            key_file_path=args.out_key,
            encrypt=args.encrypt,
        )
        and args.out_bin.exists()
    ):
        mode_str = "Encrypted" if args.encrypt else "Plaintext"
        print(f"[SUCCESS] {mode_str} NVS partition generated: {args.out_bin}")
        return 0

    return 1


if __name__ == "__main__":
    sys.exit(main())
