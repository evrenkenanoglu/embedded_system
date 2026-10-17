#!/usr/bin/env python3
"""
@file       partition_parser.py
@brief      Hardware-agnostic parser and address resolver for ESP-IDF partitions.csv tables.
            Pure utility: constraints and offsets are injected explicitly by callers.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional


@dataclass
class PartitionEntry:
    name: str
    type: str
    subtype: str
    offset: Optional[int]
    size: int
    flags: List[str]

    @property
    def is_encrypted(self) -> bool:
        return "encrypted" in [f.strip().lower() for f in self.flags]


class PartitionTableParser:
    """
    Parses standard ESP-IDF partitions.csv.
    Size queries require only the CSV file.
    Offset resolution requires layout parameters to be provided by the caller.
    """

    def __init__(
        self,
        partitions_csv_path: Path,
        partition_table_offset: Optional[int] = None,
        bootloader_offset: Optional[int] = None,
        app_alignment: Optional[int] = None,
        data_alignment: Optional[int] = None,
        partition_table_size: Optional[int] = None,
    ):
        self.csv_path = Path(partitions_csv_path).resolve()
        self.partition_table_offset = partition_table_offset
        self.bootloader_offset = bootloader_offset
        self.app_alignment = app_alignment
        self.data_alignment = data_alignment
        self.partition_table_size = partition_table_size

        self.partitions: Dict[str, PartitionEntry] = {}
        self._parse()

    def _parse(self) -> None:
        if not self.csv_path.exists():
            raise FileNotFoundError(f"Partition table CSV missing: {self.csv_path}")

        can_resolve_offsets = (
            self.partition_table_offset is not None
            and self.app_alignment is not None
            and self.data_alignment is not None
            and self.partition_table_size is not None
        )

        current_offset = (
            (self.partition_table_offset + self.partition_table_size)
            if can_resolve_offsets
            else None
        )

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
                flags = (
                    [f.strip() for f in parts[5].split(":")]
                    if len(parts) > 5 and parts[5]
                    else []
                )

                size = int(size_str, 0)
                offset = None

                if offset_str:
                    offset = int(offset_str, 0)
                    if current_offset is not None:
                        current_offset = offset + size
                elif can_resolve_offsets and current_offset is not None:
                    alignment = (
                        self.app_alignment if ptype == "app" else self.data_alignment
                    )
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
                    flags=flags,
                )

    def get_partition(self, name: str) -> PartitionEntry:
        if name not in self.partitions:
            raise KeyError(
                f"Partition '{name}' not found. Available: {list(self.partitions.keys())}"
            )
        return self.partitions[name]

    def get_size(self, name: str) -> int:
        """Retrieves partition size in bytes (only requires partitions.csv)."""
        return self.get_partition(name).size

    def get_offset(self, name: str) -> int:
        """Retrieves resolved partition offset (requires caller to have supplied layout parameters)."""
        partition = self.get_partition(name)
        if partition.offset is None:
            raise ValueError(
                f"Cannot resolve offset for partition '{name}': layout constraints "
                f"(partition_table_offset, app_alignment, data_alignment, partition_table_size) "
                f"were not supplied to PartitionTableParser by the caller."
            )
        return partition.offset

    def generate_flash_args(self, binary_mapping: Dict[str, Path]) -> List[str]:
        """Generates esptool flash arguments (requires caller to have supplied layout parameters)."""
        flash_args: List[str] = []

        for target, bin_path in binary_mapping.items():
            if not bin_path.exists():
                raise FileNotFoundError(
                    f"Binary file for '{target}' missing: {bin_path}"
                )

            if target == "__bootloader__":
                if self.bootloader_offset is None:
                    raise ValueError(
                        "Cannot resolve bootloader offset: bootloader_offset was not supplied by caller."
                    )
                offset = self.bootloader_offset
            elif target == "__partition_table__":
                if self.partition_table_offset is None:
                    raise ValueError(
                        "Cannot resolve partition table offset: partition_table_offset was not supplied by caller."
                    )
                offset = self.partition_table_offset
            else:
                offset = self.get_offset(target)

            flash_args.extend([hex(offset), str(bin_path.resolve())])

        return flash_args
