#!/usr/bin/env python3
"""
@file       partition_parser.py
@brief      Hardware-agnostic parser and address resolver for ESP-IDF partitions.csv tables.
            Pure utility: constraints and offsets are injected explicitly by callers.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import csv
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

        with open(self.csv_path, "r", encoding="utf-8", newline="") as f:
            for line_number, row in enumerate(csv.reader(f), start=1):
                if not row or not any(field.strip() for field in row):
                    continue

                parts = [field.strip() for field in row]
                if parts[0].startswith("#"):
                    continue
                if len(parts) < 5:
                    raise ValueError(
                        f"Invalid partition CSV row {line_number}: expected at least "
                        f"5 columns, got {len(parts)}."
                    )

                name = parts[0]
                ptype = parts[1]
                subtype = parts[2]
                offset_str = parts[3]
                size_str = parts[4]
                if not name or not ptype or not subtype:
                    raise ValueError(
                        f"Invalid partition CSV row {line_number}: name, type, and "
                        "subtype are required."
                    )
                if name in self.partitions:
                    raise ValueError(
                        f"Duplicate partition name '{name}' in {self.csv_path} "
                        f"(row {line_number})."
                    )
                flags = (
                    [f.strip() for f in parts[5].split(":")]
                    if len(parts) > 5 and parts[5]
                    else []
                )

                try:
                    size = int(size_str, 0)
                    offset_value = int(offset_str, 0) if offset_str else None
                except ValueError as exc:
                    raise ValueError(
                        f"Invalid numeric offset or size in partition CSV row "
                        f"{line_number}: {exc}"
                    ) from exc
                if size <= 0:
                    raise ValueError(
                        f"Partition '{name}' has invalid non-positive size {size}."
                    )
                if offset_value is not None and offset_value < 0:
                    raise ValueError(
                        f"Partition '{name}' has invalid negative offset "
                        f"{offset_value}."
                    )

                offset = None

                if offset_value is not None:
                    offset = offset_value
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

        if not self.partitions:
            raise ValueError(
                f"Partition table CSV contains no partitions: {self.csv_path}"
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

    def validate_geometry(self, flash_size: int, sector_size: int = 1) -> None:
        """Validate resolved partition ranges against flash and erase-sector geometry."""
        if flash_size <= 0:
            raise ValueError(f"Flash size must be positive, got {flash_size}.")
        if sector_size <= 0:
            raise ValueError(f"Erase-sector size must be positive, got {sector_size}.")

        ranges = []
        for partition in self.partitions.values():
            if partition.offset is None:
                raise ValueError(
                    f"Cannot validate partition '{partition.name}': its offset is "
                    "unresolved."
                )
            start = partition.offset
            end = start + partition.size
            if end > flash_size:
                raise ValueError(
                    f"Partition '{partition.name}' ends at {hex(end)}, beyond "
                    f"configured flash size {hex(flash_size)}."
                )
            sector_start = start - start % sector_size
            sector_end = ((end + sector_size - 1) // sector_size) * sector_size
            if sector_end > flash_size:
                raise ValueError(
                    f"Partition '{partition.name}' erase range ends at "
                    f"{hex(sector_end)}, beyond configured flash size {hex(flash_size)}."
                )
            ranges.append((sector_start, sector_end, partition.name))

        ranges.sort()
        for previous, current in zip(ranges, ranges[1:], strict=False):
            if current[0] < previous[1]:
                raise ValueError(
                    f"Partitions '{previous[2]}' and '{current[2]}' overlap after "
                    f"erase-sector rounding ({hex(previous[0])}-{hex(previous[1])} "
                    f"and {hex(current[0])}-{hex(current[1])})."
                )

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
