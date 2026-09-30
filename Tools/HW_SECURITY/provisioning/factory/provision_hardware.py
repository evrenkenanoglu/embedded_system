#!/usr/bin/env python3
"""
@file       provision_hardware.py
@brief      Platform-agnostic silicon provisioning delegator.
            Accepts an IToolchain instance to execute platform-specific operations.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import sys
from pathlib import Path
from typing import Dict, Any, List

from factory.partition_parser import PartitionTableParser


def read_chip_mac(port: str, baud: int, toolchain: Any) -> str:
    """Queries hardware device identifier via provided platform toolchain."""
    return toolchain.read_device_id(c=None, port=port, baud=baud)


def burn_efuse_key(port: str, baud: int, block: str, key_file: Path, purpose: str, dry_run: bool, toolchain: Any) -> None:
    """Burns a cryptographic key via provided platform toolchain."""
    toolchain.burn_key(
        c=None, port=port, baud=baud, slot=block,
        key_file=key_file, purpose=purpose, dry_run=dry_run
    )


def protect_efuse_key(port: str, baud: int, block: str, read_protect: bool, write_protect: bool, dry_run: bool, toolchain: Any) -> None:
    """Applies permanent hardware protection locks via provided platform toolchain."""
    toolchain.protect_key(
        c=None, port=port, baud=baud, slot=block,
        read_protect=read_protect, write_protect=write_protect, dry_run=dry_run
    )


def burn_efuse_register(port: str, baud: int, register_name: str, value: str, dry_run: bool, toolchain: Any) -> None:
    """Burns hardware control register or security bit via provided platform toolchain."""
    toolchain.burn_register(
        c=None, port=port, baud=baud, register_name=register_name,
        value=value, dry_run=dry_run
    )


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
    toolchain: Any
) -> None:
    """Delegates partition flashing to the provided platform toolchain."""
    flash_args = parser.generate_flash_args(binary_mapping)
    toolchain.flash_layout(
        c=None, port=port, baud=baud, chip=chip,
        flash_mode=flash_mode, flash_freq=flash_freq, flash_size=flash_size,
        flash_args=flash_args, dry_run=dry_run
    )


def wait_for_device_connection(port: str, baud: int, timeout_sec: float, toolchain: Any) -> str:
    """Waits for device connection via provided platform toolchain."""
    return toolchain.wait_for_device_connection(c=None, port=port, baud=baud, timeout_sec=timeout_sec)


def wait_for_device_disconnection(port: str, check_interval_sec: float, toolchain: Any) -> None:
    """Waits for device disconnection via provided platform toolchain."""
    toolchain.wait_for_device_disconnection(c=None, port=port, check_interval_sec=check_interval_sec)
