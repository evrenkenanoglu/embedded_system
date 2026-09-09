import os
import platform
from pathlib import Path
from typing import Any
from invoke import Context
from core import CommandSerializer
from embedded_system.Tasks.toolchains.base import BaseToolchain

IS_WINDOWS = platform.system() == "Windows"


class EspIdfToolchain(BaseToolchain):
    """Cross-platform ESP-IDF execution adapter."""

    def __init__(self, config: Any) -> None:
        super().__init__(config)
        self.work_dir = Path(getattr(self.config.paths, "workspace_dir", ".")).resolve()
        self.activation_script = getattr(
            self.config, "esp_idf_activate_script", "activate_esp-5.4-matter_1.4.2.sh"
        )

    def _get_serializer(self) -> CommandSerializer:
        work_posix = self.work_dir.as_posix()
        script = self.activation_script

        # Skip sourcing host activation scripts if inside Docker or IDF_PATH is already active
        in_container = os.path.exists("/.dockerenv") or "IDF_PATH" in os.environ

        if not in_container and script:
            if IS_WINDOWS:
                prefix = [f'if exist "{self.work_dir}\\{script}" call "{self.work_dir}\\{script}"']
            else:
                prefix = [f'if [ -f "{work_posix}/{script}" ]; then source "{work_posix}/{script}"; fi']
        else:
            prefix = []

        return CommandSerializer(
            prefix_commands=prefix,
            env=getattr(self.config, "env", None),
        )

    def _build(
        self,
        c: Context,
        target: str,
        image_bin: str,
        dry_run: bool,
        opts: str,
    ) -> None:
        work_posix = self.work_dir.as_posix()
        out_bin = image_bin if image_bin else "build/factory.bin"

        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add("rm -rf build sdkconfig")
        serializer.add(f"export IDF_TARGET={target}")
        serializer.add(f'export SDKCONFIG_DEFAULTS="{work_posix}/sdkconfig.defaults"')
        serializer.add("idf.py build", extra=opts)
        serializer.add(f'cd "{work_posix}/build" && esptool.py --chip {target} merge_bin -o {out_bin} @flash_args')
        serializer.run(c, dry_run=dry_run)

    def _flash(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        port_flag = f"-p {port}" if port else ""
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"idf.py {port_flag} flash".strip(), extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _flash_ota(self, c: Context, port: str, ota_port: int, dry_run: bool, opts: str) -> None:
        test_script = (self.work_dir / "tests" / "hil" / "test_ota_trigger.py").as_posix()
        serializer = self._get_serializer()
        serializer.add(f'pytest "{test_script}" --port={port} --ota-port={ota_port}', extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _monitor(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        port_flag = f"-p {port}" if port else ""
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"idf.py {port_flag} monitor".strip(), extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _erase(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        port_flag = f"-p {port}" if port else ""
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add(f"idf.py {port_flag} erase-flash".strip(), extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _menuconfig(self, c: Context, dry_run: bool, opts: str) -> None:
        work_posix = self.work_dir.as_posix()
        serializer = self._get_serializer()
        serializer.add(f'cd "{work_posix}"')
        serializer.add("idf.py menuconfig", extra=opts)
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