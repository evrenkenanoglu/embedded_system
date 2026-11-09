import csv
import json
import os
import platform
import re
import runpy
import shlex
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from core import CommandSerializer
from invoke import Context

from embedded_system.Tools.HW_SECURITY.provisioning.factory.partition_parser import (
    PartitionTableParser,
)
from embedded_system.Tools.TOOLCHAIN.base import BaseToolchain

IS_WINDOWS = platform.system() == "Windows"


class EspIdfToolchain(BaseToolchain):
    """ESP-IDF toolchain implementing universal hooks and ESP-specific tasks."""

    RUNTIME_REWRITTEN_FILES = {"ota_data_initial.bin"}

    def __init__(self, config: Any) -> None:
        super().__init__(config)

        # Support both dictionary configuration (from main.py) and object configuration (from Invoke)
        if isinstance(self.config, dict):
            ws = self.config.get("paths", {}).get("workspace_dir", ".")
        else:
            paths_obj = getattr(self.config, "paths", None)
            ws = getattr(paths_obj, "workspace_dir", ".") if paths_obj else "."

        self.work_dir = Path(ws).resolve()
        self.activation_script = self._cfg(
            "idf_activate_script", "activate_esp-5.4-matter_1.4.2.sh"
        )
        self.idf_py = self._resolve_idf_py_command()

    def _ctx(self, c: Optional[Context]) -> Context:
        """Guarantees a valid Invoke Context even when invoked from standalone scripts."""
        return c if c is not None else Context()

    def _cfg(self, key: str, default: Any = "") -> Any:
        """Retrieves a key from the 'esp32' section across dict and object schemas."""
        if isinstance(self.config, dict):
            section = self.config.get("esp32", {})
        else:
            section = getattr(self.config, "esp32", {})

        if isinstance(section, dict):
            val = section.get(key)
        else:
            val = getattr(section, key, None)
        return val if val is not None else default

    def _get_path(self, key: str, default: str = "") -> str:
        """Retrieves a path string from the 'paths' section across dict and object schemas."""
        if isinstance(self.config, dict):
            val = self.config.get("paths", {}).get(key, default)
        else:
            paths_obj = getattr(self.config, "paths", None)
            val = getattr(paths_obj, key, default) if paths_obj else default
        return str(val) if val else default

    def _get_default_target(self) -> str:
        if isinstance(self.config, dict):
            tp = self.config.get("target_platform") or self.config.get(
                "target", {}
            ).get("chip", "esp32")
        else:
            tp = getattr(self.config, "target_platform", "esp32")
        return str(self._cfg("target", tp)).strip()

    def _get_default_port(self) -> str:
        return str(self._cfg("port", "")).strip()

    def _resolve_idf_py_command(self) -> str:
        """Run idf.py with ESP-IDF's own interpreter so a project venv never shadows it.

        export.sh leaves an already-listed venv first on PATH, so plain `idf.py`
        would resolve the venv's python. The interpreter comes from the ESP-IDF
        export that the container already sources.
        """
        env_path = os.environ.get("IDF_PYTHON_ENV_PATH", "").strip()
        idf_path = os.environ.get("IDF_PATH", "").strip()
        if IS_WINDOWS or not env_path or not idf_path:
            return "idf.py"
        python = Path(env_path) / "bin" / "python"
        if not python.exists():
            return "idf.py"
        return f'"{python.as_posix()}" "{Path(idf_path).as_posix()}/tools/idf.py"'

    def _get_serializer(self) -> CommandSerializer:
        in_container = os.path.exists("/.dockerenv") or "IDF_PATH" in os.environ

        if not in_container and self.activation_script:
            script_path = Path(self.activation_script)
            resolved = (
                script_path
                if script_path.is_absolute()
                else (self.work_dir / script_path).resolve()
            )
            script_posix = resolved.as_posix()

            if IS_WINDOWS:
                prefix = [f'if exist "{resolved}" call "{resolved}"']
            else:
                prefix = [
                    f'if [ -f "{script_posix}" ]; then source "{script_posix}"; fi'
                ]
        else:
            prefix = []

        env = (
            self.config.get("env", None)
            if isinstance(self.config, dict)
            else getattr(self.config, "env", None)
        )

        return CommandSerializer(
            prefix_commands=prefix,
            env=env,
        )

    # --- GENERIC ABSTRACT HOOKS ---

    def _build(
        self,
        c: Optional[Context],
        target: str,
        image_bin: str,
        dry_run: bool,
        opts: str,
    ) -> None:
        work_posix = self.work_dir.as_posix()
        resolved_target = target or self._get_default_target()

        raw_defaults = self._cfg("sdkconfig_defaults", "sdkconfig.defaults")
        defaults_path = Path(raw_defaults)
        resolved_defaults = (
            defaults_path
            if defaults_path.is_absolute()
            else (self.work_dir / defaults_path).resolve()
        )

        raw_hw = self._cfg(
            "sdkconfig_hardware",
            self._get_path("sdkconfig_hardware", "sdkconfig.hardware"),
        )
        hw_path = Path(raw_hw) if raw_hw else Path("sdkconfig.hardware")
        resolved_hw = (
            hw_path if hw_path.is_absolute() else (self.work_dir / hw_path).resolve()
        )

        if resolved_hw.exists():
            defaults_val = f"{resolved_defaults.as_posix()};{resolved_hw.as_posix()}"
        else:
            defaults_val = resolved_defaults.as_posix()

        if isinstance(self.config, dict):
            proj_cfg = self.config.get("project", {})
        else:
            proj_cfg = getattr(self.config, "project", {})

        if isinstance(proj_cfg, dict):
            ver_str = proj_cfg.get("version", "0.0.1-dev")
            ver_num = proj_cfg.get("version_number", 1)
        else:
            ver_str = getattr(proj_cfg, "version", "0.0.1-dev")
            ver_num = getattr(proj_cfg, "version_number", 1)

        out_name = (
            Path(image_bin).name
            if image_bin and str(image_bin).strip()
            else "factory.bin"
        )

        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')

        build_cmd = (
            f"{self.idf_py} "
            f"-DIDF_TARGET={resolved_target} "
            f"-DSDKCONFIG_DEFAULTS='{defaults_val}' "
            f"-DPROJECT_VER='{ver_str}' "
            f"-DPROJECT_VER_NUMBER={ver_num} "
            f"build merge-bin -o {out_name}"
        )
        serializer.add(build_cmd, extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _flash(self, c: Optional[Context], port: str, dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        resolved_port = port or self._get_default_port()
        port_flag = f"-p {resolved_port}" if resolved_port else ""

        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"{self.idf_py} {port_flag} flash".strip(), extra=opts)
        serializer.add(self._verify_flash_command(port_flag, dry_run))
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _flash_selected(
        self,
        c: Optional[Context],
        partitions: str,
        group: str,
        port: str,
        dry_run: bool,
        opts: str,
    ) -> None:
        if opts.strip():
            raise ValueError(
                "Extra --opts are not accepted with selected flashing; use the "
                "validated command settings so reset, encryption, or write targets "
                "cannot be overridden."
            )

        selected_names = self._resolve_flash_selection(partitions, group)
        regions, metadata = self._preflight_flash_selection(selected_names)
        if not regions:
            raise ValueError("Selected flash operation has no artifact regions.")

        hardware = self._section("hardware")
        target = self._section("target")
        chip = self._required_string(self._field(target, "chip"), "target.chip").lower()
        baud = self._cfg("flash_baudrate", self._field(hardware, "flash_baud", ""))
        flash_mode = self._required_string(
            self._field(hardware, "flash_mode"), "hardware.flash_mode"
        )
        flash_freq = self._required_string(
            self._field(hardware, "flash_freq"), "hardware.flash_freq"
        )
        flash_size = self._required_string(
            self._field(hardware, "flash_size"), "hardware.flash_size"
        )
        metadata_settings = metadata.get("flash_settings", {})
        if not isinstance(metadata_settings, dict):
            raise ValueError("Generated flash metadata has invalid flash_settings.")
        extra_esptool_args = metadata.get("extra_esptool_args", {})
        if not isinstance(extra_esptool_args, dict):
            raise ValueError("Generated flash metadata has invalid extra_esptool_args.")
        if "write_flash_args" not in metadata:
            raise ValueError(
                "Generated flash metadata is missing required write_flash_args."
            )
        write_flash_args = metadata["write_flash_args"]
        if not isinstance(write_flash_args, list) or any(
            not isinstance(arg, str) for arg in write_flash_args
        ):
            raise ValueError("Generated flash metadata has invalid write_flash_args.")
        if "--encrypt" in write_flash_args:
            raise ValueError(
                "Selected flashing does not use hardware flash encryption; refusing "
                "metadata that requests encrypted writes."
            )
        configured_settings = {}
        for setting, configured in (
            ("flash_mode", flash_mode),
            ("flash_freq", flash_freq),
            ("flash_size", flash_size),
        ):
            if setting not in metadata_settings:
                raise ValueError(
                    f"Generated flash metadata is missing required {setting}."
                )
            generated = self._required_string(
                metadata_settings[setting], f"generated flash metadata {setting}"
            )
            if generated.lower() not in (
                configured.lower(),
                "keep",
            ):
                raise ValueError(
                    f"Generated flash metadata {setting}={generated!r} does not "
                    f"match the project SSoT value {configured!r}."
                )
            if generated.lower() == "keep":
                configured_settings[setting] = "keep"
            else:
                configured_settings[setting] = configured

        args = [
            sys.executable,
            "-m",
            "esptool",
            "--chip",
            chip,
        ]
        if port:
            args.extend(["--port", port])
        if baud:
            args.extend(["--baud", str(baud)])
        args.extend(
            [
                "--before",
                "default_reset",
                "--after",
                "no_reset",
                "write_flash",
                "--verify",
                "--flash_mode",
                configured_settings["flash_mode"],
                "--flash_freq",
                configured_settings["flash_freq"],
                "--flash_size",
                configured_settings["flash_size"],
            ]
        )
        for offset, path in regions:
            args.extend([hex(offset), str(path)])

        serializer = self._get_serializer()
        if IS_WINDOWS:
            change_dir = f"cd /d {subprocess.list2cmdline([str(self.work_dir)])}"
            command = subprocess.list2cmdline(args)
        else:
            change_dir = f"cd {shlex.quote(self.work_dir.as_posix())}"
            command = shlex.join(args)
        serializer.add(change_dir)
        serializer.add(command)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _section(self, name: str) -> Any:
        if isinstance(self.config, dict):
            return self.config.get(name, {})
        return getattr(self.config, name, {})

    @staticmethod
    def _field(section: Any, name: str, default: Any = None) -> Any:
        if isinstance(section, dict):
            return section.get(name, default)
        return getattr(section, name, default)

    @staticmethod
    def _required_string(value: Any, label: str) -> str:
        resolved = str(value).strip() if value is not None else ""
        if not resolved or resolved.startswith("{"):
            raise ValueError(f"Unresolved or missing configuration value: {label}.")
        return resolved

    @staticmethod
    def _parse_flash_size(value: Any) -> int:
        text = str(value).strip().upper()
        match = re.fullmatch(r"([0-9]+)\s*(B|KB|MB|GB)?", text)
        if not match:
            raise ValueError(
                f"Invalid hardware.flash_size {value!r}; expected a byte count or "
                "a value such as 8MB."
            )
        scale = {"B": 1, "KB": 1024, "MB": 1024**2, "GB": 1024**3}
        size = int(match.group(1)) * scale.get(match.group(2) or "B", 1)
        if size <= 0:
            raise ValueError("hardware.flash_size must be positive.")
        return size

    @staticmethod
    def _parse_geometry_value(value: Any, label: str) -> int:
        try:
            result = int(str(value).strip(), 0)
        except (TypeError, ValueError) as exc:
            raise ValueError(f"Invalid {label} value {value!r}.") from exc
        if result < 0:
            raise ValueError(f"{label} must not be negative.")
        return result

    def _resolve_flash_selection(self, partitions: str, group: str) -> List[str]:
        partition_arg = str(partitions or "").strip()
        group_arg = str(group or "").strip()
        if partition_arg and group_arg:
            raise ValueError("Choose either --partitions or --group, not both.")

        layout = self._section("flash_layout")
        artifacts = self._field(layout, "artifacts", {})
        groups = self._field(layout, "groups", {})
        if not isinstance(artifacts, dict) or not artifacts:
            raise ValueError("flash_layout.artifacts is missing or invalid in SSoT.")
        if not isinstance(groups, dict):
            raise ValueError("flash_layout.groups must be a mapping in SSoT.")

        if group_arg:
            if group_arg not in groups:
                available = ", ".join(sorted(groups)) or "(none)"
                raise ValueError(
                    f"Unknown flash group {group_arg!r}. Configured groups: {available}."
                )
            group_entries = groups[group_arg]
            if not isinstance(group_entries, list) or not group_entries:
                raise ValueError(
                    f"Configured flash group {group_arg!r} must contain at least "
                    "one artifact name."
                )
            names = [str(name).strip() for name in group_entries]
        else:
            if not partition_arg:
                raise ValueError("Selected flashing requires --partitions or --group.")
            names = [name.strip() for name in partition_arg.split(",")]

        if any(not name for name in names):
            raise ValueError(
                "Flash selection contains an empty name; provide one or more "
                "comma-separated configured names."
            )
        if len(set(names)) != len(names):
            duplicates = sorted({name for name in names if names.count(name) > 1})
            raise ValueError(
                f"Flash selection contains duplicate name(s): {', '.join(duplicates)}."
            )

        unknown = [name for name in names if name not in artifacts]
        if unknown:
            available = ", ".join(sorted(artifacts))
            raise ValueError(
                f"Unknown flash artifact(s): {', '.join(unknown)}. Configured "
                f"artifacts: {available}."
            )
        return names

    def _preflight_flash_selection(
        self, names: List[str]
    ) -> Tuple[List[Tuple[int, Path]], Dict[str, Any]]:
        layout = self._section("flash_layout")
        paths = self._section("paths")
        artifacts = self._field(layout, "artifacts", {})
        definitions = self._field(layout, "partitions", [])
        if not isinstance(definitions, list) or not definitions:
            raise ValueError("flash_layout.partitions is missing or invalid in SSoT.")

        partition_table_offset = self._parse_geometry_value(
            self._field(layout, "partition_table_offset"),
            "flash_layout.partition_table_offset",
        )
        partition_table_size = self._parse_geometry_value(
            self._field(layout, "partition_table_size"),
            "flash_layout.partition_table_size",
        )
        bootloader_offset = self._parse_geometry_value(
            self._field(layout, "bootloader_offset"),
            "flash_layout.bootloader_offset",
        )
        sector_size = self._parse_geometry_value(
            self._field(layout, "data_alignment"), "flash_layout.data_alignment"
        )
        app_alignment = self._parse_geometry_value(
            self._field(layout, "app_alignment"), "flash_layout.app_alignment"
        )
        flash_size = self._parse_flash_size(
            self._field(self._section("hardware"), "flash_size")
        )
        if not partition_table_size or not sector_size or not app_alignment:
            raise ValueError(
                "Partition-table size and flash alignments must be positive in SSoT."
            )
        partition_table_end = partition_table_offset + partition_table_size
        if (
            bootloader_offset >= partition_table_offset
            or partition_table_end > flash_size
            or bootloader_offset % sector_size
            or partition_table_offset % sector_size
            or partition_table_end % sector_size
        ):
            raise ValueError(
                "Bootloader/partition-table geometry is invalid or outside configured "
                "flash; check flash_layout offsets, table size, and data_alignment."
            )

        csv_value = self._field(paths, "partitions_csv")
        csv_path = Path(self._required_string(csv_value, "paths.partitions_csv"))
        if not csv_path.is_absolute():
            csv_path = (self.work_dir / csv_path).resolve()
        if not csv_path.is_file():
            raise FileNotFoundError(
                f"Generated partition metadata is missing: {csv_path}. Run "
                "'inv es.build-config.partitions' to generate it from "
                "configs/config_project.yaml before selected flashing."
            )

        parser = PartitionTableParser(
            csv_path,
            partition_table_offset=partition_table_offset,
            bootloader_offset=bootloader_offset,
            app_alignment=app_alignment,
            data_alignment=sector_size,
            partition_table_size=partition_table_size,
        )
        configured_by_name: Dict[str, Dict[str, Any]] = {}
        expected_offsets: Dict[str, int] = {}
        current_offset = partition_table_end
        for definition in definitions:
            if not isinstance(definition, dict):
                raise ValueError(
                    "Every flash_layout.partitions entry must be a mapping."
                )
            name = self._required_string(
                definition.get("name"), "flash_layout.partitions[].name"
            )
            if name in configured_by_name:
                raise ValueError(f"Duplicate configured physical partition {name!r}.")
            configured_by_name[name] = definition

            explicit_offset = definition.get("offset")
            alignment = (
                app_alignment
                if str(definition.get("type", "")).lower() == "app"
                else sector_size
            )
            if explicit_offset not in (None, ""):
                expected_offset = self._parse_geometry_value(
                    explicit_offset, f"flash_layout.partitions.{name}.offset"
                )
            else:
                expected_offset = (
                    (current_offset + alignment - 1) // alignment
                ) * alignment
            if expected_offset % alignment:
                raise ValueError(
                    f"Configured partition {name!r} offset {hex(expected_offset)} "
                    f"is not aligned to {hex(alignment)} bytes."
                )
            expected_offsets[name] = expected_offset
            current_offset = expected_offset + self._parse_geometry_value(
                definition.get("size"),
                f"flash_layout.partitions.{name}.size",
            )

        if set(parser.partitions) != set(configured_by_name):
            missing = sorted(set(configured_by_name) - set(parser.partitions))
            extra = sorted(set(parser.partitions) - set(configured_by_name))
            raise ValueError(
                "Generated partition CSV does not match flash_layout.partitions: "
                f"missing={missing}, extra={extra}. Regenerate it with "
                "'inv es.build-config.partitions'."
            )
        for name, definition in configured_by_name.items():
            generated = parser.get_partition(name)
            expected_size = self._parse_geometry_value(
                definition.get("size"), f"flash_layout.partitions.{name}.size"
            )
            expected_type = str(definition.get("type", "")).strip()
            expected_subtype = str(definition.get("subtype", "")).strip()
            expected_flags = sorted(
                flag.strip().lower()
                for flag in str(definition.get("flags", "")).split(":")
                if flag.strip()
            )
            if (
                generated.size != expected_size
                or generated.type.lower() != expected_type.lower()
                or generated.subtype.lower() != expected_subtype.lower()
                or sorted(flag.lower() for flag in generated.flags if flag)
                != expected_flags
                or parser.get_offset(name) != expected_offsets[name]
            ):
                raise ValueError(
                    f"Generated partition metadata for {name!r} does not match "
                    "flash_layout in configs/config_project.yaml. Regenerate it "
                    "with 'inv es.build-config.partitions'."
                )

        table_artifacts = [
            artifact
            for artifact in artifacts.values()
            if isinstance(artifact, dict) and artifact.get("kind") == "partition_table"
        ]
        if len(table_artifacts) != 1:
            raise ValueError(
                "flash_layout.artifacts must define exactly one partition_table "
                "artifact so the compiled table can be validated."
            )
        self._validate_compiled_partition_table(
            csv_path,
            partition_table_offset,
            flash_size,
            table_artifacts[0].get("path"),
        )
        parser.validate_geometry(flash_size=flash_size, sector_size=sector_size)
        physical_ranges = [
            (bootloader_offset, partition_table_offset, "bootloader region"),
            (partition_table_offset, partition_table_end, "partition table region"),
        ]
        physical_ranges.extend(
            (
                parser.get_offset(name),
                parser.get_offset(name) + parser.get_size(name),
                name,
            )
            for name in parser.partitions
        )
        rounded_ranges = []
        for start, end, label in physical_ranges:
            if start < 0 or end <= start or end > flash_size:
                raise ValueError(
                    f"Configured flash region {label!r} ({hex(start)}-{hex(end)}) "
                    f"is outside flash size {hex(flash_size)}."
                )
            erase_start = start - start % sector_size
            erase_end = ((end + sector_size - 1) // sector_size) * sector_size
            if erase_end > flash_size:
                raise ValueError(
                    f"Erase-rounded flash region {label!r} ends beyond "
                    f"{hex(flash_size)}."
                )
            rounded_ranges.append((erase_start, erase_end, label))
        rounded_ranges.sort()
        for previous, current in zip(rounded_ranges, rounded_ranges[1:], strict=False):
            if current[0] < previous[1]:
                raise ValueError(
                    f"Configured flash regions {previous[2]!r} and {current[2]!r} "
                    "overlap after erase-sector rounding."
                )

        metadata_path = Path(
            self._required_string(
                self._field(paths, "flash_metadata"), "paths.flash_metadata"
            )
        )
        if not metadata_path.is_absolute():
            metadata_path = (self.work_dir / metadata_path).resolve()
        if not metadata_path.is_file():
            raise FileNotFoundError(
                f"Generated flash metadata is missing: {metadata_path}. Run "
                "'inv es.platform.build' to generate ESP-IDF flash metadata "
                "before selected flashing."
            )
        try:
            metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ValueError(
                f"Cannot read generated flash metadata {metadata_path}: {exc}"
            ) from exc
        flash_files = metadata.get("flash_files")
        if not isinstance(flash_files, dict) or not flash_files:
            raise ValueError(
                f"Generated flash metadata has no valid flash_files mapping: "
                f"{metadata_path}."
            )
        extra_esptool_args = metadata.get("extra_esptool_args", {})
        if not isinstance(extra_esptool_args, dict):
            raise ValueError("Generated flash metadata has invalid extra_esptool_args.")
        metadata_chip = self._required_string(
            extra_esptool_args.get("chip"), "generated flash metadata chip"
        )
        target_chip = self._field(self._section("target"), "chip")
        if metadata_chip.lower() != str(target_chip).lower():
            raise ValueError(
                f"Generated flash metadata targets {metadata_chip!r}, but the "
                f"project SSoT targets {target_chip!r}."
            )

        resolved_artifacts: Dict[str, Tuple[Path, int, int, int, bool]] = {}
        for name, artifact in artifacts.items():
            if not isinstance(artifact, dict):
                raise ValueError(f"flash_layout.artifacts.{name} must be a mapping.")
            kind = str(artifact.get("kind", "")).strip()
            metadata_required = artifact.get("metadata_required", False)
            if not isinstance(metadata_required, bool):
                raise ValueError(
                    f"flash_layout.artifacts.{name}.metadata_required must be a "
                    "boolean."
                )
            image_path_value = artifact.get("path")
            image_path = Path(
                self._required_string(
                    image_path_value, f"flash_layout.artifacts.{name}.path"
                )
            )
            if not image_path.is_absolute():
                image_path = (self.work_dir / image_path).resolve()

            if kind == "bootloader":
                offset = bootloader_offset
                region_start = bootloader_offset
                region_end = partition_table_offset
            elif kind == "partition_table":
                offset = partition_table_offset
                region_start = partition_table_offset
                region_end = partition_table_end
            elif kind == "partition":
                if name not in parser.partitions:
                    raise ValueError(
                        f"Configured artifact {name!r} refers to no physical "
                        "partition in generated metadata."
                    )
                partition = parser.get_partition(name)
                if partition.is_encrypted:
                    raise ValueError(
                        f"Configured artifact {name!r} uses a hardware-encrypted "
                        "partition, but selected flashing writes plaintext images."
                    )
                offset = parser.get_offset(name)
                region_start = offset
                region_end = offset + partition.size
            else:
                raise ValueError(
                    f"Unsupported kind {kind!r} for flash artifact {name!r}; "
                    "use 'bootloader', 'partition_table', or 'partition'."
                )

            resolved_artifacts[name] = (
                image_path,
                offset,
                region_start,
                region_end,
                metadata_required,
            )

        flash_files_by_path: Dict[Path, List[int]] = {}
        flash_files_by_name: Dict[str, List[Tuple[Path, int]]] = {}
        build_dir = Path(self._get_path("build_dir", self.work_dir / "build"))
        if not build_dir.is_absolute():
            build_dir = (self.work_dir / build_dir).resolve()
        for raw_offset, raw_path in flash_files.items():
            try:
                generated_offset = int(str(raw_offset), 0)
            except ValueError as exc:
                raise ValueError(
                    f"Invalid offset {raw_offset!r} in generated flash metadata."
                ) from exc
            generated_path = Path(str(raw_path))
            if not generated_path.is_absolute():
                generated_path = build_dir / generated_path
            generated_path = generated_path.resolve()
            flash_files_by_path.setdefault(generated_path, []).append(generated_offset)
            flash_files_by_name.setdefault(generated_path.name, []).append(
                (generated_path, generated_offset)
            )

        selected_regions: List[Tuple[int, Path]] = []
        selected_ranges = []
        for name in names:
            (
                image_path,
                offset,
                region_start,
                region_end,
                metadata_required,
            ) = resolved_artifacts[name]
            if not image_path.is_file():
                raise FileNotFoundError(
                    f"Flash artifact {name!r} is missing: {image_path}."
                )
            image_size = image_path.stat().st_size
            if image_size <= 0:
                raise ValueError(f"Flash artifact {name!r} is empty: {image_path}.")
            if image_size > region_end - region_start:
                raise ValueError(
                    f"Flash artifact {name!r} is {image_size} bytes, larger than "
                    f"its configured region ({region_end - region_start} bytes)."
                )

            image_end = offset + image_size
            erase_start = offset - offset % sector_size
            erase_end = ((image_end + sector_size - 1) // sector_size) * sector_size
            region_erase_start = region_start - region_start % sector_size
            region_erase_end = (
                (region_end + sector_size - 1) // sector_size
            ) * sector_size
            if (
                offset < 0
                or image_end > flash_size
                or erase_end > flash_size
                or erase_start < region_erase_start
                or erase_end > region_erase_end
            ):
                raise ValueError(
                    f"Flash artifact {name!r} at {hex(offset)} with size "
                    f"{image_size} exceeds its configured region or flash geometry."
                )

            metadata_matches = flash_files_by_path.get(image_path.resolve(), [])
            same_name = flash_files_by_name.get(image_path.name, [])
            if not metadata_matches and same_name:
                raise ValueError(
                    f"Generated flash metadata references {image_path.name!r} at "
                    f"{same_name[0][0]}, not configured artifact path {image_path}."
                )
            if not metadata_matches and metadata_required:
                raise ValueError(
                    f"Generated flash metadata does not include required build "
                    f"artifact {image_path}."
                )
            if metadata_matches and (
                len(metadata_matches) != 1 or metadata_matches[0] != offset
            ):
                raise ValueError(
                    f"Generated flash metadata offset for {image_path.name!r} "
                    f"does not match configured offset {hex(offset)}."
                )

            for prior_start, prior_end, prior_name in selected_ranges:
                if erase_start < prior_end and prior_start < erase_end:
                    raise ValueError(
                        f"Selected artifacts {prior_name!r} and {name!r} overlap "
                        "after erase-sector rounding."
                    )
            selected_ranges.append((erase_start, erase_end, name))
            selected_regions.append((offset, image_path.resolve()))

        selected_regions.sort(key=lambda region: region[0])
        return selected_regions, metadata

    def _validate_compiled_partition_table(
        self,
        csv_path: Path,
        partition_table_offset: int,
        flash_size: int,
        raw_table_path: Any,
    ) -> None:
        """Compare the built binary table against the generated CSV via IDF's parser."""
        table_path = Path(
            self._required_string(
                raw_table_path, "flash_layout.artifacts.partition_table.path"
            )
        )
        if not table_path.is_absolute():
            table_path = (self.work_dir / table_path).resolve()
        if not table_path.is_file():
            raise FileNotFoundError(
                f"Compiled partition table is missing: {table_path}. Rebuild with "
                "'inv es.platform.build' after generating the current table."
            )

        idf_root = self._resolve_idf_root()
        parser_path = idf_root / "components" / "partition_table" / "gen_esp32part.py"
        if not parser_path.is_file():
            raise FileNotFoundError(
                f"ESP-IDF partition parser is missing: {parser_path}. Check the "
                "selected SDK path or rebuild the project manifest."
            )

        try:
            sdk_globals = runpy.run_path(str(parser_path))
        except (OSError, ImportError, SyntaxError) as exc:
            raise ValueError(
                f"Unable to load ESP-IDF partition parser {parser_path}: {exc}"
            ) from exc

        partition_table_type = sdk_globals.get("PartitionTable")
        input_error_type = sdk_globals.get("InputError")
        validation_error_type = sdk_globals.get("ValidationError")
        if (
            not isinstance(partition_table_type, type)
            or not isinstance(input_error_type, type)
            or not issubclass(input_error_type, BaseException)
            or not isinstance(validation_error_type, type)
            or not issubclass(validation_error_type, input_error_type)
        ):
            raise ValueError(
                f"ESP-IDF partition parser {parser_path} has an unsupported API."
            )
        from_binary = getattr(partition_table_type, "from_binary", None)
        from_csv = getattr(partition_table_type, "from_csv", None)
        if not callable(from_binary) or not callable(from_csv):
            raise ValueError(
                f"ESP-IDF partition parser {parser_path} has an unsupported API."
            )
        parser_globals = getattr(from_binary, "__globals__", None)
        if not isinstance(parser_globals, dict):
            raise ValueError(
                f"ESP-IDF partition parser {parser_path} has no configurable "
                "module globals."
            )

        try:
            parser_globals["offset_part_table"] = partition_table_offset
            compiled_data = table_path.read_bytes()
            compiled_table = from_binary(compiled_data)
            configured_table = from_csv(csv_path.read_text(encoding="utf-8"))
            compiled_table.verify()
            configured_table.verify()
            compiled_table.verify_size_fits(flash_size)
            configured_table.verify_size_fits(flash_size)
            configured_data = configured_table.to_binary()
        except (
            OSError,
            UnicodeError,
            ValueError,
            csv.Error,
            input_error_type,
            validation_error_type,
        ) as exc:
            raise ValueError(
                f"Unable to validate compiled partition table {table_path} against "
                f"{csv_path}: {exc}. Regenerate and rebuild with "
                "'inv es.build-config.partitions && inv es.platform.build'."
            ) from exc

        if compiled_data != configured_data:
            raise ValueError(
                f"Compiled partition table {table_path} is stale relative to "
                f"{csv_path} and flash_layout in configs/config_project.yaml. "
                "Regenerate and rebuild with 'inv es.build-config.partitions && "
                "inv es.platform.build' before flashing."
            )

    def _resolve_idf_root(self) -> Path:
        """Resolve the SDK from IDF_PATH or the generated project build manifest."""
        explicit_path = os.environ.get("IDF_PATH", "").strip()
        if explicit_path:
            idf_root = Path(explicit_path).expanduser().resolve()
            if not idf_root.is_dir():
                raise FileNotFoundError(
                    f"IDF_PATH does not identify an SDK directory: {idf_root}."
                )
            return idf_root

        build_dir_value = self._get_path("build_dir", str(self.work_dir / "build"))
        build_dir = Path(self._required_string(build_dir_value, "paths.build_dir"))
        if not build_dir.is_absolute():
            build_dir = (self.work_dir / build_dir).resolve()
        manifest_path = build_dir / "project_description.json"
        if not manifest_path.is_file():
            raise FileNotFoundError(
                "IDF_PATH is unset and the generated build manifest is missing: "
                f"{manifest_path}. Build the project to generate it or set IDF_PATH."
            )
        try:
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        except (OSError, UnicodeError, json.JSONDecodeError) as exc:
            raise ValueError(
                f"Unable to read ESP-IDF build manifest {manifest_path}: {exc}. "
                "Set IDF_PATH or regenerate the project build manifest."
            ) from exc
        if not isinstance(manifest, dict):
            raise ValueError(
                f"ESP-IDF build manifest {manifest_path} must contain a JSON object."
            )
        manifest_idf_path = manifest.get("idf_path")
        if not isinstance(manifest_idf_path, str) or not manifest_idf_path.strip():
            raise ValueError(
                f"ESP-IDF build manifest {manifest_path} has no usable idf_path. "
                "Set IDF_PATH or regenerate the project build manifest."
            )
        idf_root = Path(manifest_idf_path.strip()).expanduser()
        if not idf_root.is_absolute():
            idf_root = (self.work_dir / idf_root).resolve()
        if not idf_root.is_dir():
            raise FileNotFoundError(
                f"ESP-IDF build manifest {manifest_path} points to a missing SDK "
                f"directory: {idf_root}."
            )
        return idf_root

    def _verify_flash_command(self, port_flag: str, dry_run: bool) -> str:
        """Read back each flashed region and compare it with its build file.

        The explicit readback guards the image bytes before boot. ota_data_initial
        is skipped because the bootloader and app rewrite it on the first boot.
        """
        build_dir = self.work_dir / "build"
        args_path = build_dir / "flasher_args.json"
        if not args_path.exists():
            if dry_run:
                return (
                    f"echo 'verify skipped in dry run: {args_path} not generated yet'"
                )
            raise FileNotFoundError(f"Cannot verify flash, missing {args_path}")

        flash_files = json.loads(args_path.read_text())["flash_files"]
        regions = [
            f"{offset} {file}"
            for offset, file in flash_files.items()
            if Path(file).name not in self.RUNTIME_REWRITTEN_FILES
        ]
        chip = getattr(self, "chip", None) or getattr(self, "target", None)
        chip_flag = f"--chip {chip}" if chip else ""
        python_executable = (
            subprocess.list2cmdline([sys.executable])
            if IS_WINDOWS
            else shlex.quote(sys.executable)
        )
        return (
            f'cd "{build_dir.as_posix()}" && '
            f"{python_executable} -m esptool {port_flag} {chip_flag} "
            f"--before default_reset --after no_reset verify_flash {' '.join(regions)}"
        )

    def _monitor(
        self, c: Optional[Context], port: str, dry_run: bool, opts: str
    ) -> None:
        work_posix = self.work_dir.as_posix()
        resolved_port = port or self._get_default_port()
        port_flag = f"-p {resolved_port}" if resolved_port else ""

        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"{self.idf_py} {port_flag} monitor".strip(), extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _test(self, c: Optional[Context], dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"{self.idf_py} set-target linux")
        serializer.add(f"{self.idf_py} build")
        serializer.add("./build/unit_test_app", extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _clean(self, c: Optional[Context], dry_run: bool) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"{self.idf_py} clean")
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _clean_all(self, c: Optional[Context], dry_run: bool) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"{self.idf_py} fullclean")
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _configure(self, c: Optional[Context], dry_run: bool, opts: str) -> None:
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir.as_posix()}"')
        serializer.add(f"{self.idf_py} menuconfig", extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _erase(self, c: Optional[Context], port: str, dry_run: bool, opts: str) -> None:
        port_flag = f"-p {port}" if port else ""
        chip = getattr(self, "chip", None) or getattr(self, "target", None)
        chip_flag = f"--chip {chip}" if chip else ""
        baud = getattr(self, "flash_baud", None) or getattr(self, "baud", None)
        baud_flag = f"-b {baud}" if baud else ""

        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir.as_posix()}"')

        # Bypass idf.py injecting --no-stub during Secure Boot / Flash Encryption:
        cmd = f"{sys.executable} -m esptool {port_flag} {baud_flag} {chip_flag} erase_flash".strip()
        serializer.add(cmd, extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _provision(
        self, c: Optional[Context], config_path: str, step: str, dry_run: bool
    ) -> None:
        script_raw = self._get_path("provision_hardware_script")
        script = Path(script_raw).resolve()
        serializer = self._get_serializer()
        cmd = f'{sys.executable} "{script}" --step {step} --config "{config_path}"'
        if dry_run:
            cmd += " --dry-run"
        serializer.add(cmd)
        serializer.run(self._ctx(c), dry_run=dry_run)

    # --- SILICON HARDWARE PROVISIONING HOOKS ---

    def _read_device_id(self, c: Optional[Context], port: str, baud: int) -> str:
        baud_rate = baud or int(self._cfg("flash_baudrate", 460800))
        cmd = [
            sys.executable,
            "-m",
            "esptool",
            "--port",
            port,
            "--baud",
            str(baud_rate),
            "read_mac",
        ]
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode == 0:
            for line in res.stdout.splitlines():
                if "MAC:" in line:
                    return line.split("MAC:")[-1].strip().replace(":", "-").upper()

        # Handle Secure Download Mode gracefully (ROM blocks read_mac)
        combined_output = (res.stderr or "") + (res.stdout or "")
        if "Secure Download Mode" in combined_output:
            print(
                "[*] Device is in Secure Download Mode (eFuses/MAC read-protected by silicon ROM)."
            )
            return "SECURE_LOCKED_DEVICE"

        return "UNKNOWN_DEVICE"

    def _get_virt_flags(self) -> str:
        """Returns virtual eFuse CLI flags when running in software simulation mode."""
        chip = getattr(self, "chip", "esp32s3")
        if getattr(self, "_virtual_mode", False) and getattr(
            self, "_virtual_state_file", None
        ):
            return f'--chip {chip} --virt --path-efuse-file "{self._virtual_state_file.resolve()}"'
        return f"--chip {chip}"

    def _is_secure_download_mode(
        self, c: Optional[Context], port: str, baud: int
    ) -> bool:
        """Checks if target chip is locked in Secure Download Mode."""
        if getattr(self, "_virtual_mode", False):
            return False

        serializer = self._get_serializer()
        chip_flag = f"--chip {getattr(self, 'chip', 'esp32s3')}"
        cmd = f"{sys.executable} -m espefuse {chip_flag} --port {port} --baud {baud} summary"
        serializer.add(cmd)
        ctx = self._ctx(c)
        try:
            res = ctx.run(serializer.serialize(), hide=True, warn=True)
            output = (res.stdout or "") + (res.stderr or "")
            return "Secure Download Mode is enabled" in output
        except Exception:
            return False

    def _get_efuse_summary(self, c: Optional[Context], port: str, baud: int) -> str:
        """Queries physical or virtual eFuse summary table."""
        serializer = self._get_serializer()
        virt_flags = self._get_virt_flags()
        cmd = f"{sys.executable} -m espefuse {virt_flags} --port {port} --baud {baud} summary".strip()
        serializer.add(" ".join(cmd.split()))
        ctx = self._ctx(c)
        try:
            res = ctx.run(serializer.serialize(), hide=True, warn=True)
            output = (res.stdout or "") + (res.stderr or "")
            return output if res and res.ok else ""
        except Exception:
            return ""

    def _burn_key(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        slot: str,
        key_file: Path,
        purpose: str,
        dry_run: bool,
    ) -> None:
        if not key_file.exists():
            raise FileNotFoundError(f"Key file missing for {slot}: {key_file}")

        is_virt = getattr(self, "_virtual_mode", False)

        # Idempotency and hardware lock checks
        if not dry_run and not is_virt:
            if self._is_secure_download_mode(c, port, baud):
                print(
                    f"[*] Secure Download Mode active (silicon eFuses permanently locked). Skipping {slot} burn."
                )
                return

            summary = self._get_efuse_summary(c, port, baud)
            slot_index = slot.replace("BLOCK_KEY", "").replace("BLOCK", "").strip()
            purpose_key = f"KEY_PURPOSE_{slot_index}"

            for line in summary.splitlines():
                if slot in line or (
                    f"BLOCK{int(slot_index) + 4}" in line
                    if slot_index.isdigit()
                    else False
                ):
                    if (
                        "??" in line
                        or "read-protected" in line.lower()
                        or "read-disabled" in line.lower()
                    ):
                        print(
                            f"[*] eFuse {slot} is already programmed and read-protected. Skipping burn."
                        )
                        return

                if purpose_key in line and purpose in line:
                    print(
                        f"[*] eFuse {slot} purpose already set to {purpose}. Skipping burn."
                    )
                    return

        serializer = self._get_serializer()
        virt_flags = self._get_virt_flags()
        cmd = f'{sys.executable} -m espefuse {virt_flags} --port {port} --baud {baud} --do-not-confirm burn_key {slot} "{key_file.resolve()}" {purpose}'.strip()
        serializer.add(" ".join(cmd.split()))
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _protect_key(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        slot: str,
        read_protect: bool,
        write_protect: bool,
        dry_run: bool,
    ) -> None:
        is_virt = getattr(self, "_virtual_mode", False)

        # Idempotency and hardware lock checks
        if not dry_run and not is_virt:
            if self._is_secure_download_mode(c, port, baud):
                print(
                    f"[*] Secure Download Mode active. Skipping {slot} protection lock."
                )
                return

            summary = self._get_efuse_summary(c, port, baud)
            slot_index = slot.replace("BLOCK_KEY", "").replace("BLOCK", "").strip()

            for line in summary.splitlines():
                if slot in line or (
                    f"BLOCK{int(slot_index) + 4}" in line
                    if slot_index.isdigit()
                    else False
                ):
                    if (
                        "??" in line
                        or "read-protected" in line.lower()
                        or "read-disabled" in line.lower()
                    ):
                        read_protect = False
                    if (
                        "-/W" in line
                        or "-/-" in line
                        or "R/-" in line
                        or "write-protected" in line.lower()
                    ):
                        write_protect = False

            if not read_protect and not write_protect:
                print(
                    f"[*] eFuse block {slot} is already protected as requested. Skipping lock."
                )
                return

        serializer = self._get_serializer()
        virt_flags = self._get_virt_flags()

        if read_protect:
            cmd = f"{sys.executable} -m espefuse {virt_flags} --port {port} --baud {baud} --do-not-confirm read_protect_efuse {slot}".strip()
            serializer.add(" ".join(cmd.split()))

        if write_protect:
            cmd = f"{sys.executable} -m espefuse {virt_flags} --port {port} --baud {baud} --do-not-confirm write_protect_efuse {slot}".strip()
            serializer.add(" ".join(cmd.split()))

        serializer.run(self._ctx(c), dry_run=dry_run)

    def _burn_register(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        register_name: str,
        value: str,
        dry_run: bool,
    ) -> None:
        is_virt = getattr(self, "_virtual_mode", False)

        if not dry_run and not is_virt:
            if self._is_secure_download_mode(c, port, baud):
                print(
                    f"[*] Secure Download Mode active. Skipping {register_name} burn."
                )
                return

        serializer = self._get_serializer()
        virt_flags = self._get_virt_flags()
        cmd = f"{sys.executable} -m espefuse {virt_flags} --port {port} --baud {baud} --do-not-confirm burn_efuse {register_name} {value}".strip()
        serializer.add(" ".join(cmd.split()))
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _flash_layout(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        chip: str,
        flash_mode: str,
        flash_freq: str,
        flash_size: str,
        flash_args: List[str],
        dry_run: bool,
    ) -> None:
        serializer = self._get_serializer()
        cmd = (
            f"{sys.executable} -m esptool --chip {chip} --port {port} --baud {baud} "
            f"--before default_reset --after hard_reset write_flash --force "
            f"--flash_mode {flash_mode} --flash_freq {flash_freq} --flash_size {flash_size} "
            + " ".join(flash_args)
        )
        serializer.add(cmd)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _wait_for_device_connection(
        self, c: Optional[Context], port: str, baud: int, timeout_sec: float
    ) -> str:
        print(f"\n[FIXTURE READY] Connect target device to '{port}'...")
        start_time = time.time()
        while time.time() - start_time < timeout_sec:
            try:
                cmd = [
                    sys.executable,
                    "-m",
                    "esptool",
                    "--port",
                    port,
                    "--baud",
                    str(baud),
                    "--connect-attempts",
                    "1",
                    "read_mac",
                ]
                res = subprocess.run(cmd, capture_output=True, text=True, check=False)
                if res.returncode == 0:
                    for line in res.stdout.splitlines():
                        if "MAC:" in line:
                            mac = (
                                line.split("MAC:")[-1].strip().replace(":", "-").upper()
                            )
                            print(f"[*] Detected target device! MAC: {mac}")
                            return mac
            except Exception:
                pass
            time.sleep(0.5)
        raise TimeoutError(f"No device connected to {port} within {timeout_sec}s.")

    def _wait_for_device_disconnection(
        self, c: Optional[Context], port: str, check_interval_sec: float
    ) -> None:
        print(f"\n[DISCONNECT REQUIRED] Unplug device from '{port}'...")
        while True:
            try:
                cmd = [
                    sys.executable,
                    "-m",
                    "esptool",
                    "--port",
                    port,
                    "--connect-attempts",
                    "1",
                    "chip_id",
                ]
                res = subprocess.run(cmd, capture_output=True, text=True, check=False)
                if res.returncode != 0:
                    print("[*] Device disconnected. Ready for next unit.\n")
                    break
            except Exception:
                break
            time.sleep(check_interval_sec)

    # --- ESP32-SPECIFIC TASK HELPERS ---

    def flash_ota(
        self,
        c: Optional[Context],
        port: str = "",
        ota_port: int = 8032,
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        resolved_port = port or self._get_default_port()
        port_label = resolved_port or "AUTO"
        with self._stage("📡", "FLASH OTA", f"{port_label}:{ota_port}"):
            test_script = (
                self.work_dir / "tests" / "hil" / "test_ota_trigger.py"
            ).as_posix()
            serializer = self._get_serializer()
            serializer.add(
                f'pytest "{test_script}" --port={resolved_port} --ota-port={ota_port}',
                extra=opts,
            )
            serializer.run(self._ctx(c), dry_run=dry_run)
