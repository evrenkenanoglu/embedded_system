#!/usr/bin/env python3
"""
@file       provision_hardware.py
@brief      Generic silicon provisioning engine executing data-driven eFuse burning and flash flashing.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import subprocess
import sys
from pathlib import Path
from typing import Dict, Any, List

SCRIPT_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPT_DIR))

from factory.audit_logger import AuditLogger
from factory.partition_parser import PartitionTableParser


def run_command(cmd: List[str], desc: str, dry_run: bool = False) -> subprocess.CompletedProcess:
    """Executes a shell command with structured logging."""
    print(f"\n[*] {desc}")
    print(f"    Command: {' '.join(cmd)}")
    if dry_run:
        print("    [DRY-RUN] Execution bypassed.")
        return subprocess.CompletedProcess(args=cmd, returncode=0, stdout="", stderr="")

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[ERROR] Command failed with code {result.returncode}:\n{result.stderr}", file=sys.stderr)
        raise RuntimeError(f"Step '{desc}' failed.")
    return result


def read_chip_mac(port: str, baud: int) -> str:
    """Queries target chip MAC address via esptool.py."""
    cmd = [sys.executable, "-m", "esptool", "--port", port, "--baud", str(baud), "read_mac"]
    res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if res.returncode != 0:
        raise RuntimeError(f"Failed to communicate with device on {port}:\n{res.stderr}")
    for line in res.stdout.splitlines():
        if "MAC:" in line:
            return line.split("MAC:")[-1].strip().replace(":", "-").upper()
    return "UNKNOWN_MAC"


def burn_efuse_key(port: str, baud: int, block: str, key_file: Path, purpose: str, dry_run: bool) -> None:
    """Burns an encryption key into an eFuse block."""
    if not key_file.exists():
        raise FileNotFoundError(f"Key file for {block} missing: {key_file}")
    cmd = [
        sys.executable, "-m", "espefuse",
        "--port", port,
        "--baud", str(baud),
        "--do-not-confirm",
        "burn_key", block, str(key_file.resolve()), purpose
    ]
    run_command(cmd, f"Burning eFuse key block {block} ({purpose})", dry_run)


def burn_efuse_register(port: str, baud: int, register_name: str, value: str, dry_run: bool) -> None:
    """Burns an arbitrary eFuse register value or lock bit."""
    cmd = [
        sys.executable, "-m", "espefuse",
        "--port", port,
        "--baud", str(baud),
        "--do-not-confirm",
        "burn_efuse", register_name, str(value)
    ]
    run_command(cmd, f"Burning eFuse {register_name} = {value}", dry_run)


def flash_dynamic_layout(
    port: str,
    baud: int,
    chip: str,
    flash_mode: str,
    flash_freq: str,
    flash_size: str,
    parser: PartitionTableParser,
    binary_mapping: Dict[str, Path],
    dry_run: bool
) -> None:
    """Constructs dynamic write_flash arguments from parsed partition table and flashes the device."""
    flash_args = parser.generate_flash_args(binary_mapping)

    cmd = [
        sys.executable, "-m", "esptool",
        "--chip", chip,
        "--port", port,
        "--baud", str(baud),
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash",
        "-z",
        "--flash_mode", flash_mode,
        "--flash_freq", flash_freq,
        "--flash_size", flash_size,
    ] + flash_args

    run_command(cmd, "Flashing dynamic partition layout", dry_run)