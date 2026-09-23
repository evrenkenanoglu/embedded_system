#!/usr/bin/env python3
"""
@file       provision_hardware.py
@brief      Production-grade silicon provisioning engine with automated key burning,
            eFuse read/write protection locks, and partition flashing.
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


def run_command(
    cmd: List[str], desc: str, dry_run: bool = False
) -> subprocess.CompletedProcess:
    """
    Executes a shell command with structured logging and error handling.

    :param cmd: Argument list for the command.
    :param desc: Operational description for audit trails.
    :param dry_run: When True, logs the command without executing.
    :return: CompletedProcess instance.
    :raises RuntimeError: On non-zero return codes.
    """
    print(f"\n[*] {desc}")
    print(f"    Command: {' '.join(cmd)}")
    if dry_run:
        print("    [DRY-RUN] Execution bypassed.")
        return subprocess.CompletedProcess(args=cmd, returncode=0, stdout="", stderr="")

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(
            f"[ERROR] Step '{desc}' failed with exit code {result.returncode}:\n{result.stderr}",
            file=sys.stderr,
        )
        raise RuntimeError(f"Provisioning step failed: {desc}")
    return result


def read_chip_mac(port: str, baud: int) -> str:
    """
    Queries the target ESP32-S3 MAC address via esptool.py.

    :param port: Serial device path (e.g. /dev/ttyUSB0, COM3).
    :param baud: Communication baud rate.
    :return: Formatted MAC address (XX-XX-XX-XX-XX-XX).
    """
    cmd = [
        sys.executable,
        "-m",
        "esptool",
        "--port",
        port,
        "--baud",
        str(baud),
        "read_mac",
    ]
    res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if res.returncode != 0:
        raise RuntimeError(
            f"Failed to communicate with device on {port}:\n{res.stderr}"
        )
    for line in res.stdout.splitlines():
        if "MAC:" in line:
            return line.split("MAC:")[-1].strip().replace(":", "-").upper()
    return "UNKNOWN_MAC"


def burn_efuse_key(
    port: str, baud: int, block: str, key_file: Path, purpose: str, dry_run: bool
) -> None:
    """
    Burns a 256-bit cryptographic key into a designated physical eFuse block.

    :param port: Serial device path.
    :param baud: Flashing baud rate.
    :param block: Target eFuse block (e.g. BLOCK_KEY0, BLOCK_KEY1).
    :param key_file: Path to the binary key file.
    :param purpose: Silicon key purpose (FLASH_ENCRYPTION, SECURE_BOOT_DIGEST0).
    :param dry_run: Flag to simulate execution.
    """
    if not key_file.exists():
        raise FileNotFoundError(f"Key file for {block} missing: {key_file}")

    cmd = [
        sys.executable,
        "-m",
        "espefuse",
        "--port",
        port,
        "--baud",
        str(baud),
        "--do-not-confirm",
        "burn_key",
        block,
        str(key_file.resolve()),
        purpose,
    ]
    run_command(cmd, f"Burning eFuse key block {block} ({purpose})", dry_run)


def protect_efuse_key(
    port: str,
    baud: int,
    block: str,
    read_protect: bool,
    write_protect: bool,
    dry_run: bool,
) -> None:
    """
    Applies permanent hardware read and write protection to an eFuse key block.

    :param port: Serial device path.
    :param baud: Communication baud rate.
    :param block: Target eFuse block name.
    :param read_protect: When True, permanently disables read access by software.
    :param write_protect: When True, permanently locks the block against modification.
    :param dry_run: Flag to simulate execution.
    """
    if read_protect:
        cmd = [
            sys.executable,
            "-m",
            "espefuse",
            "--port",
            port,
            "--baud",
            str(baud),
            "--do-not-confirm",
            "read_protect_efuse",
            block,
        ]
        run_command(cmd, f"Read-protecting eFuse {block} (Hardware AES only)", dry_run)

    if write_protect:
        cmd = [
            sys.executable,
            "-m",
            "espefuse",
            "--port",
            port,
            "--baud",
            str(baud),
            "--do-not-confirm",
            "write_protect_efuse",
            block,
        ]
        run_command(cmd, f"Write-protecting eFuse {block} (Silicon Lock)", dry_run)


def burn_efuse_register(
    port: str, baud: int, register_name: str, value: str, dry_run: bool
) -> None:
    """
    Burns an arbitrary eFuse register, version counter, or security lock bit.

    :param port: Serial device path.
    :param baud: Communication baud rate.
    :param register_name: eFuse register identifier.
    :param value: Value to program into the register.
    :param dry_run: Flag to simulate execution.
    """
    cmd = [
        sys.executable,
        "-m",
        "espefuse",
        "--port",
        port,
        "--baud",
        str(baud),
        "--do-not-confirm",
        "burn_efuse",
        register_name,
        str(value),
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
    dry_run: bool,
) -> None:
    """
    Dynamically resolves partition offsets and flashes the complete image layout via esptool.

    :param port: Serial device path.
    :param baud: Flashing baud rate.
    :param chip: SoC target variant (e.g. esp32s3).
    :param flash_mode: SPI flash mode (dio, qio).
    :param flash_freq: Flash frequency (80m, 40m).
    :param flash_size: Total chip flash size (8MB, 16MB).
    :param parser: Instantiated PartitionTableParser.
    :param binary_mapping: Map of { partition_name: binary_file_path }.
    :param dry_run: Flag to simulate execution.
    """
    flash_args = parser.generate_flash_args(binary_mapping)

    cmd = [
        sys.executable,
        "-m",
        "esptool",
        "--chip",
        chip,
        "--port",
        port,
        "--baud",
        str(baud),
        "--before",
        "default_reset",
        "--after",
        "hard_reset",
        "write_flash",
        "-z",
        "--flash_mode",
        flash_mode,
        "--flash_freq",
        flash_freq,
        "--flash_size",
        flash_size,
    ] + flash_args

    run_command(cmd, "Flashing dynamic partition layout", dry_run)
