import os
from pathlib import Path
from typing import Any
from invoke import Context
from core import CommandSerializer
from embedded_system.Tasks.toolchains.base import BaseToolchain


class EspIdfToolchain(BaseToolchain):
    """ESP-IDF implementation conforming to BaseToolchain."""

    def __init__(self, config: Any) -> None:
        super().__init__(config)
        self.work_dir = Path(getattr(self.config.paths, "workspace_dir", ".")).resolve()
        self.activation_script = getattr(
            self.config, "esp_idf_activate_script", "activate_esp-5.4-matter_1.4.2.sh"
        )

    def _get_serializer(self) -> CommandSerializer:
        prefix = [f"source {self.activation_script}"] if self.activation_script else []
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
        out_bin = image_bin if image_bin else "build/factory.bin"
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir}"')
        serializer.add("rm -rf build sdkconfig")
        serializer.add(f"export IDF_TARGET={target}")
        serializer.add(f'export SDKCONFIG_DEFAULTS="{self.work_dir}/sdkconfig.defaults"')
        serializer.add("idf.py build", extra=opts)
        serializer.add(f'cd "{self.work_dir}/build" && esptool.py --chip {target} merge_bin -o {out_bin} @flash_args')
        serializer.run(c, dry_run=dry_run)

    def _flash(
        self,
        c: Context,
        port: str,
        dry_run: bool,
        opts: str,
    ) -> None:
        port_flag = f"-p {port}" if port else ""
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir}"')
        serializer.add(f"idf.py {port_flag} flash".strip(), extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _flash_ota(
        self,
        c: Context,
        port: str,
        ota_port: int,
        dry_run: bool,
        opts: str,
    ) -> None:
        test_script = self.work_dir / "tests" / "hil" / "test_ota_trigger.py"
        serializer = self._get_serializer()
        serializer.add(f'pytest "{test_script}" --port={port} --ota-port={ota_port}', extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _monitor(
        self,
        c: Context,
        port: str,
        dry_run: bool,
        opts: str,
    ) -> None:
        port_flag = f"-p {port}" if port else ""
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir}"')
        serializer.add(f"idf.py {port_flag} monitor".strip(), extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _erase(
        self,
        c: Context,
        port: str,
        dry_run: bool,
        opts: str,
    ) -> None:
        port_flag = f"-p {port}" if port else ""
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir}"')
        serializer.add(f"idf.py {port_flag} erase-flash".strip(), extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _menuconfig(
        self,
        c: Context,
        dry_run: bool,
        opts: str,
    ) -> None:
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir}"')
        serializer.add("idf.py menuconfig", extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _test(
        self,
        c: Context,
        dry_run: bool,
        opts: str,
    ) -> None:
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir}"')
        serializer.add("idf.py set-target linux")
        serializer.add("idf.py build")
        serializer.add("./build/unit_test_app", extra=opts)
        serializer.run(c, dry_run=dry_run)

    def _clean(
        self,
        c: Context,
        dry_run: bool,
    ) -> None:
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir}"')
        serializer.add("idf.py clean")
        serializer.run(c, dry_run=dry_run)

    def _clean_all(
        self,
        c: Context,
        dry_run: bool,
    ) -> None:
        serializer = self._get_serializer()
        serializer.add(f'cd "{self.work_dir}"')
        serializer.add("idf.py fullclean")
        serializer.run(c, dry_run=dry_run)