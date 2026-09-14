import os
import platform
from pathlib import Path
from typing import Any
from invoke import Context
from core import CommandSerializer
from embedded_system.Tasks.toolchains.base import BaseToolchain

IS_WINDOWS = platform.system() == "Windows"


class EspIdfToolchain(BaseToolchain):
    """ESP-IDF toolchain implementing universal hooks and ESP-specific tasks."""

    def __init__(self, config: Any) -> None:
        super().__init__(config)
        self.work_dir = Path(getattr(self.config.paths, "workspace_dir", ".")).resolve()
        self.activation_script = self._cfg("idf_activate_script", "activate_esp-5.4-matter_1.4.2.sh")

    def _cfg(self, key: str, default: Any = "") -> Any:
        section = getattr(self.config, "esp32", {})
        if isinstance(section, dict):
            val = section.get(key)
        else:
            val = getattr(section, key, None)
        return val if val is not None else default

    def _get_default_target(self) -> str:
        return str(self._cfg("target", getattr(self.config, "target_platform", "esp32"))).strip()

    def _get_default_port(self) -> str:
        return str(self._cfg("port", "")).strip()

    def _get_serializer(self) -> CommandSerializer:
        in_container = os.path.exists("/.dockerenv") or "IDF_PATH" in os.environ

        if not in_container and self.activation_script:
            script_path = Path(self.activation_script)
            resolved = script_path if script_path.is_absolute() else (self.work_dir / script_path).resolve()
            script_posix = resolved.as_posix()

            if IS_WINDOWS:
                prefix = [f'if exist "{resolved}" call "{resolved}"']
            else:
                prefix = [f'if [ -f "{script_posix}" ]; then source "{script_posix}"; fi']
        else:
            prefix = []

        return CommandSerializer(
            prefix_commands=prefix,
            env=getattr(self.config, "env", None),
        )

    # --- GENERIC ABSTRACT HOOKS ---

    def _build(self, c: Context, target: str, image_bin: str, dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        resolved_target = target or self._get_default_target()

        # 1. Resolve base sdkconfig.defaults
        raw_defaults = self._cfg("sdkconfig_defaults", "sdkconfig.defaults")
        defaults_path = Path(raw_defaults)
        resolved_defaults = defaults_path if defaults_path.is_absolute() else (self.work_dir / defaults_path).resolve()

        # 2. Resolve hardware SSoT overlay if present (sdkconfig.hardware)
        paths_cfg = getattr(self.config, "paths", {})
        raw_hw = self._cfg("sdkconfig_hardware", getattr(paths_cfg, "sdkconfig_hardware", "sdkconfig.hardware"))
        hw_path = Path(raw_hw) if raw_hw else Path("sdkconfig.hardware")
        resolved_hw = hw_path if hw_path.is_absolute() else (self.work_dir / hw_path).resolve()

        # Merge defaults via native ESP-IDF semicolon delimiter
        if resolved_hw.exists():
            defaults_val = f"{resolved_defaults.as_posix()};{resolved_hw.as_posix()}"
        else:
            defaults_val = resolved_defaults.as_posix()

        # 3. Pull SSoT project versions for CMake and Matter OTA image tool
        proj_cfg = getattr(self.config, "project", {})
        if isinstance(proj_cfg, dict):
            ver_str = proj_cfg.get("version", "0.0.1-dev")
            ver_num = proj_cfg.get("version_number", 1)
        else:
            ver_str = getattr(proj_cfg, "version", "0.0.1-dev")
            ver_num = getattr(proj_cfg, "version_number", 1)

        # 4. Resolve output binary name
        out_name = Path(image_bin).name if image_bin and str(image_bin).strip() else "factory.bin"

        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')

        build_cmd = (
            f"idf.py "
            f"-DIDF_TARGET={resolved_target} "
            f"-DSDKCONFIG_DEFAULTS='{defaults_val}' "
            f"-DPROJECT_VER='{ver_str}' "
            f"-DPROJECT_VER_NUMBER={ver_num} "
            f"build merge-bin -o {out_name}"
        )
        serializer.add(build_cmd, extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _flash(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        resolved_port = port or self._get_default_port()
        port_flag = f"-p {resolved_port}" if resolved_port else ""

        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"idf.py {port_flag} flash".strip(), extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _monitor(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        resolved_port = port or self._get_default_port()
        port_flag = f"-p {resolved_port}" if resolved_port else ""

        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"idf.py {port_flag} monitor".strip(), extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _test(self, c: Context, dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add("idf.py set-target linux")
        serializer.add("idf.py build")
        serializer.add("./build/unit_test_app", extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _clean(self, c: Context, dry_run: bool) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add("idf.py clean")
        serializer.run(c, dry_run=dry_run)

    def _clean_all(self, c: Context, dry_run: bool) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add("idf.py fullclean")
        serializer.run(c, dry_run=dry_run)

    # --- ESP32-SPECIFIC METHODS ---

    def menuconfig(self, c: Context, dry_run: bool = False, opts: str = "") -> None:
        with self._stage("⚙️", "MENUCONFIG"):
            work_posix = self.work_dir.as_posix()
            serializer = self._get_serializer()
            serializer.add(f'cd "{work_posix}"')
            serializer.add("idf.py menuconfig", extra=opts)
            serializer.run(c, dry_run=dry_run)

    def erase(self, c: Context, port: str = "", dry_run: bool = False, opts: str = "") -> None:
        resolved_port = port or self._get_default_port()
        port_label = resolved_port or "AUTO"
        with self._stage("🗑️", "ERASE FLASH", port_label):
            work_posix = self.work_dir.as_posix()
            port_flag = f"-p {resolved_port}" if resolved_port else ""
            serializer = self._get_serializer()
            serializer.add(f'cd "{work_posix}"')
            serializer.add(f"idf.py {port_flag} erase-flash".strip(), extra=opts)
            serializer.run(c, dry_run=dry_run)

    def flash_ota(self, c: Context, port: str = "", ota_port: int = 8032, dry_run: bool = False, opts: str = "") -> None:
        resolved_port = port or self._get_default_port()
        port_label = resolved_port or "AUTO"
        with self._stage("📡", "FLASH OTA", f"{port_label}:{ota_port}"):
            test_script = (self.work_dir / "tests" / "hil" / "test_ota_trigger.py").as_posix()
            serializer = self._get_serializer()
            serializer.add(f'pytest "{test_script}" --port={resolved_port} --ota-port={ota_port}', extra=opts)
            serializer.run(c, dry_run=dry_run)