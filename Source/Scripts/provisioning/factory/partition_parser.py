#!/usr/bin/env python3
"""
@file       partition_parser.py
@brief      Generic parser and address resolver for ESP-IDF partitions.csv tables.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import csv
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple


@dataclass
class PartitionEntry:
    name: str
    type: str
    subtype: str
    offset: int
    size: int
    flags: List[str]

    @property
    def is_encrypted(self) -> bool:
        return "encrypted" in [f.strip().lower() for f in self.flags]


class PartitionTableParser:
    """Parses standard ESP-IDF partitions.csv and resolves offsets dynamically."""

    APP_ALIGNMENT = 0x10000   # 64 KB boundary for app slots
    DATA_ALIGNMENT = 0x1000   # 4 KB sector boundary for data slots
    FIRST_PARTITION_OFFSET = 0x9000  # Default first partition offset following partition table at 0x8000

    def __init__(self, partitions_csv_path: Path):
        self.csv_path = partitions_csv_path
        self.partitions: Dict[str, PartitionEntry] = {}
        self._parse()

    def _parse(self) -> None:
        if not self.csv_path.exists():
            raise FileNotFoundError(f"Partition table CSV missing: {self.csv_path}")

        current_offset = self.FIRST_PARTITION_OFFSET

        with open(self.csv_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                # Skip comments and empty lines
                if not line or line.startswith("#"):
                    continue

                parts = [p.strip() for p in line.split(",")]
                if len(parts) < 5:
                    continue

                name = parts[0]
                ptype = parts[1]
                subtype = parts[2]
                offset_str = parts[3]
                size_str = parts[4]
                flags = [f.strip() for f in parts[5].split(":")] if len(parts) > 5 and parts[5] else []

                size = int(size_str, 0)

                # Determine alignment
                alignment = self.APP_ALIGNMENT if ptype == "app" else self.DATA_ALIGNMENT

                # Resolve Offset
                if offset_str:
                    offset = int(offset_str, 0)
                    current_offset = offset + size
                else:
                    # Align current offset to boundary
                    if current_offset % alignment != 0:
                        current_offset += alignment - (current_offset % alignment)
                    offset = current_offset
                    current_offset += size

                self.partitions[name] = PartitionEntry(
                    name=name,
                    type=ptype,
                    subtype=subtype,
                    offset=offset,
                    size=size,
                    flags=flags
                )

    def get_partition(self, name: str) -> PartitionEntry:
        """Retrieves partition metadata by name."""
        if name not in self.partitions:
            raise KeyError(f"Partition '{name}' not found in {self.csv_path.name}. Available: {list(self.partitions.keys())}")
        return self.partitions[name]

    def get_offset(self, name: str) -> int:
        """Returns the resolved flash offset for a partition."""
        return self.get_partition(name).offset

    def get_size(self, name: str) -> int:
        """Returns the resolved flash size for a partition."""
        return self.get_partition(name).size

    def generate_flash_args(self, binary_mapping: Dict[str, Path]) -> List[str]:
        """
        Constructs dynamic offset-binary arguments for esptool write_flash.
        Input mapping: { partition_name: binary_path }
        Special names: '__bootloader__' (0x0000), '__partition_table__' (0x8000)
        """
        flash_args: List[str] = []

        for target, bin_path in binary_mapping.items():
            if not bin_path.exists():
                raise FileNotFoundError(f"Binary file for '{target}' missing: {bin_path}")

            if target == "__bootloader__":
                offset = 0x0000
            elif target == "__partition_table__":
                offset = 0x8000
            else:
                offset = self.get_offset(target)

            flash_args.extend([hex(offset), str(bin_path.resolve())])

        return flash_args