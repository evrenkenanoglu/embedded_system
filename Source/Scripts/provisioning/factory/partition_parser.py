#!/usr/bin/env python3
"""
@file       partition_parser.py
@brief      Generic parser and address resolver for ESP-IDF partitions.csv tables.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List


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

    # Silicon Hardware Constraints (Non-configurable physical flash limits)
    APP_ALIGNMENT = 0x10000   # 64 KB ESP32 MMU execution boundary
    DATA_ALIGNMENT = 0x1000   # 4 KB physical flash sector erase boundary
    PARTITION_TABLE_SIZE = 0x1000  # 4 KB allocated partition table sector

    def __init__(
        self, 
        partitions_csv_path: Path, 
        partition_table_offset: int = 0x8000,
        bootloader_offset: int = 0x0000
    ):
        self.csv_path = partitions_csv_path
        self.partition_table_offset = partition_table_offset
        self.bootloader_offset = bootloader_offset
        # First partition always starts at the sector immediately following the partition table
        self.first_partition_offset = self.partition_table_offset + self.PARTITION_TABLE_SIZE
        
        self.partitions: Dict[str, PartitionEntry] = {}
        self._parse()

    def _parse(self) -> None:
        if not self.csv_path.exists():
            raise FileNotFoundError(f"Partition table CSV missing: {self.csv_path}")

        current_offset = self.first_partition_offset

        with open(self.csv_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
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
                alignment = self.APP_ALIGNMENT if ptype == "app" else self.DATA_ALIGNMENT

                # Resolve offset dynamically if not explicitly specified in CSV
                if offset_str:
                    offset = int(offset_str, 0)
                    current_offset = offset + size
                else:
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
        if name not in self.partitions:
            raise KeyError(f"Partition '{name}' not found. Available: {list(self.partitions.keys())}")
        return self.partitions[name]

    def get_offset(self, name: str) -> int:
        return self.get_partition(name).offset

    def get_size(self, name: str) -> int:
        return self.get_partition(name).size

    def generate_flash_args(self, binary_mapping: Dict[str, Path]) -> List[str]:
        flash_args: List[str] = []

        for target, bin_path in binary_mapping.items():
            if not bin_path.exists():
                raise FileNotFoundError(f"Binary file for '{target}' missing: {bin_path}")

            if target == "__bootloader__":
                offset = self.bootloader_offset
            elif target == "__partition_table__":
                offset = self.partition_table_offset
            else:
                offset = self.get_offset(target)

            flash_args.extend([hex(offset), str(bin_path.resolve())])

        return flash_args