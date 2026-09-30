import os
import platform
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, List, Optional
from invoke import Context
from core import CommandSerializer
from embedded_system.Tasks.toolchains.base import BaseToolchain

IS_WINDOWS = platform.system() == "Windows"


class EspIdfToolchain(BaseToolchain):
    """ESP-IDF toolchain implementing universal hooks and ESP-specific tasks."""

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
            f"idf.py "
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
        serializer.add(f"idf.py {port_flag} flash".strip(), extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _monitor(
        self, c: Optional[Context], port: str, dry_run: bool, opts: str
    ) -> None:
        work_posix = self.work_dir.as_posix()
        resolved_port = port or self._get_default_port()
        port_flag = f"-p {resolved_port}" if resolved_port else ""

        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"idf.py {port_flag} monitor".strip(), extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _test(self, c: Optional[Context], dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add("idf.py set-target linux")
        serializer.add("idf.py build")
        serializer.add("./build/unit_test_app", extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _clean(self, c: Optional[Context], dry_run: bool) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add("idf.py clean")
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _clean_all(self, c: Optional[Context], dry_run: bool) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add("idf.py fullclean")
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _configure(self, c: Optional[Context], dry_run: bool, opts: str) -> None:
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir.as_posix()}"')
        serializer.add("idf.py menuconfig", extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _erase(self, c: Optional[Context], port: str, dry_run: bool, opts: str) -> None:
        port_flag = f"-p {port}" if port else ""
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir.as_posix()}"')
        serializer.add(f"idf.py {port_flag} erase-flash".strip(), extra=opts)
        serializer.run(self._ctx(c), dry_run=dry_run)

    def _provision(
        self, c: Optional[Context], config_path: str, step: str, dry_run: bool
    ) -> None:
        script_raw = self._get_path("provision_hardware_script")
        script = Path(script_raw).resolve()
        serializer = self._get_serializer()
        cmd = f'python "{script}" --step {step} --config "{config_path}"'
        if dry_run:
            cmd += " --dry-run"
        serializer.add(cmd)
        serializer.run(self._ctx(c), dry_run=dry_run)

    # --- SILICON HARDWARE PROVISIONING HOOKS ---

    def _read_device_id(self, c: Optional[Context], port: str, baud: int) -> str:
        baud_rate = baud or int(self._cfg("flash_baudrate", 460800))
        cmd = [sys.executable, "-m", "esptool", "--port", port, "--baud", str(baud_rate), "read_mac"]
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode == 0:
            for line in res.stdout.splitlines():
                if "MAC:" in line:
                    return line.split("MAC:")[-1].strip().replace(":", "-").upper()

        # Handle Secure Download Mode gracefully (ROM blocks read_mac)
        combined_output = (res.stderr or "") + (res.stdout or "")
        if "Secure Download Mode" in combined_output:
            print("[*] Device is in Secure Download Mode (eFuses/MAC read-protected by silicon ROM).")
            return "SECURE_LOCKED_DEVICE"

        return "UNKNOWN_DEVICE"

    def _is_secure_download_mode(self, c: Optional[Context], port: str, baud: int) -> bool:
        """Checks if target chip is locked in Secure Download Mode."""
        serializer = self._get_serializer()
        cmd = f"python -m espefuse --port {port} --baud {baud} summary"
        serializer.add(cmd)
        ctx = self._ctx(c)
        try:
            res = ctx.run(serializer.serialize(), hide=True, warn=True)
            output = (res.stdout or "") + (res.stderr or "")
            return "Secure Download Mode is enabled" in output
        except Exception:
            return False

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

        if not dry_run:
            if self._is_secure_download_mode(c, port, baud):
                print(f"[*] Secure Download Mode active (silicon eFuses permanently locked). Skipping {slot} burn.")
                return

            summary = self._get_efuse_summary(c, port, baud)
            slot_index = slot.replace("BLOCK_KEY", "").replace("BLOCK", "").strip()
            purpose_key = f"KEY_PURPOSE_{slot_index}"

            for line in summary.splitlines():
                if slot in line or (f"BLOCK{int(slot_index) + 4}" in line if slot_index.isdigit() else False):
                    if "??" in line or "read-protected" in line.lower() or "read-disabled" in line.lower():
                        print(f"[*] eFuse {slot} is already programmed and read-protected. Skipping burn.")
                        return

                if purpose_key in line and purpose in line:
                    print(f"[*] eFuse {slot} purpose already set to {purpose}. Skipping burn.")
                    return

        serializer = self._get_serializer()
        cmd = f'python -m espefuse --port {port} --baud {baud} --do-not-confirm burn_key {slot} "{key_file.resolve()}" {purpose}'
        serializer.add(cmd)
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
        if not dry_run:
            if self._is_secure_download_mode(c, port, baud):
                print(f"[*] Secure Download Mode active. Skipping {slot} protection lock.")
                return

            summary = self._get_efuse_summary(c, port, baud)
            slot_index = slot.replace("BLOCK_KEY", "").replace("BLOCK", "").strip()

            for line in summary.splitlines():
                if slot in line or (f"BLOCK{int(slot_index) + 4}" in line if slot_index.isdigit() else False):
                    if "??" in line or "read-protected" in line.lower() or "read-disabled" in line.lower():
                        read_protect = False
                    if "-/W" in line or "-/-" in line or "R/-" in line or "write-protected" in line.lower():
                        write_protect = False

        if not read_protect and not write_protect:
            print(f"[*] eFuse block {slot} is already protected as requested. Skipping lock.")
            return

        serializer = self._get_serializer()
        if read_protect:
            serializer.add(
                f"python -m espefuse --port {port} --baud {baud} --do-not-confirm read_protect_efuse {slot}"
            )
        if write_protect:
            serializer.add(
                f"python -m espefuse --port {port} --baud {baud} --do-not-confirm write_protect_efuse {slot}"
            )
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
        if not dry_run and self._is_secure_download_mode(c, port, baud):
            print(f"[*] Secure Download Mode active. Skipping {register_name} burn.")
            return

        serializer = self._get_serializer()
        cmd = f"python -m espefuse --port {port} --baud {baud} --do-not-confirm burn_efuse {register_name} {value}"
        serializer.add(cmd)
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
            f"python -m esptool --chip {chip} --port {port} --baud {baud} "
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
