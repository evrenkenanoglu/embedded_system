import os
import sys
import time
from abc import abstractmethod
from contextlib import contextmanager
from pathlib import Path
from typing import Any, Generator, List, Optional
from invoke import Context
from invoke.exceptions import UnexpectedExit

from embedded_system.Tasks.toolchains.interface import IToolchain


class BaseToolchain(IToolchain):
    """Platform-agnostic base managing execution lifecycle, timing, and stage banners."""

    def __init__(self, config: Any) -> None:
        self.config = config

    @property
    def _use_color(self) -> bool:
        if "NO_COLOR" in os.environ:
            return False
        if os.environ.get("FORCE_COLOR") in ("1", "true"):
            return True
        return hasattr(sys.stdout, "isatty") and sys.stdout.isatty()

    def _fmt(self, text: str, code: str) -> str:
        return f"{code}{text}\033[0m" if self._use_color else text

    @contextmanager
    def _stage(
        self, emoji: str, action: str, details: str = ""
    ) -> Generator[None, None, None]:
        CYAN = "\033[1;36m"
        GREEN = "\033[1;32m"
        RED = "\033[1;31m"
        WHITE_BOLD = "\033[1;37m"

        target_info = f" ({details})" if details else ""
        header = f"{emoji} {action.upper()}{target_info}"
        width = 70
        line = "━" * width

        print(f"\n\n{self._fmt(line, CYAN)}")
        print(f"{self._fmt(' ❯ ', CYAN)}{self._fmt(header, WHITE_BOLD)}")
        print(f"{self._fmt(line, CYAN)}\n")

        start_time = time.perf_counter()
        try:
            yield
            elapsed = time.perf_counter() - start_time
            status = self._fmt(f"✅ {action.upper()} COMPLETED", GREEN)
            print(f"\n{status} [{elapsed:.2f}s]\n")
        except UnexpectedExit as err:
            elapsed = time.perf_counter() - start_time
            exit_code = err.result.returncode if err.result else "unknown"
            status = self._fmt(f"❌ {action.upper()} FAILED", RED)
            print(f"\n{status} (Exit code {exit_code}) [{elapsed:.2f}s]\n")
            raise
        except Exception as err:
            elapsed = time.perf_counter() - start_time
            status = self._fmt(f"❌ {action.upper()} FAILED", RED)
            print(f"\n{status} [{elapsed:.2f}s]: {err}\n")
            raise

    # --- SUBCLASS DEFAULTS HOOKS ---

    def _get_default_target(self) -> str:
        return ""

    def _get_default_port(self) -> str:
        return ""

    # --- PUBLIC TEMPLATE METHODS ---

    def build(
        self,
        c: Optional[Context],
        target: str = "",
        image_bin: str = "",
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        resolved_target = (
            str(target).strip()
            if target and str(target).strip()
            else self._get_default_target()
        )
        with self._stage("🔨", "BUILD", resolved_target):
            self._build(
                c,
                target=resolved_target,
                image_bin=image_bin,
                dry_run=dry_run,
                opts=opts,
            )

    def flash(
        self,
        c: Optional[Context],
        port: str = "",
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        resolved_port = (
            str(port).strip()
            if port and str(port).strip()
            else self._get_default_port()
        )
        port_label = resolved_port or "AUTO"
        with self._stage("⚡", "FLASH", port_label):
            self._flash(c, port=resolved_port, dry_run=dry_run, opts=opts)

    def monitor(
        self,
        c: Optional[Context],
        port: str = "",
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        resolved_port = (
            str(port).strip()
            if port and str(port).strip()
            else self._get_default_port()
        )
        port_label = resolved_port or "AUTO"
        with self._stage("🖥️", "MONITOR", port_label):
            self._monitor(c, port=resolved_port, dry_run=dry_run, opts=opts)

    def test(self, c: Optional[Context], dry_run: bool = False, opts: str = "") -> None:
        with self._stage("🧪", "HOST UNIT TESTS"):
            self._test(c, dry_run=dry_run, opts=opts)

    def clean(self, c: Optional[Context], dry_run: bool = False) -> None:
        with self._stage("🧹", "CLEAN"):
            self._clean(c, dry_run=dry_run)

    def clean_all(self, c: Optional[Context], dry_run: bool = False) -> None:
        with self._stage("🧹", "FULL CLEAN"):
            self._clean_all(c, dry_run=dry_run)

    def configure(
        self, c: Optional[Context], dry_run: bool = False, opts: str = ""
    ) -> None:
        with self._stage("⚙️", "CONFIGURE"):
            self._configure(c, dry_run=dry_run, opts=opts)

    def erase(
        self,
        c: Optional[Context],
        port: str = "",
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        resolved_port = str(port).strip() if port else self._get_default_port()
        with self._stage("🗑️", "ERASE FLASH", resolved_port or "AUTO"):
            self._erase(c, port=resolved_port, dry_run=dry_run, opts=opts)

    def provision(
        self,
        c: Optional[Context],
        config_path: str,
        step: str = "all",
        dry_run: bool = False,
    ) -> None:
        with self._stage("🔒", "SILICON PROVISIONING", f"step: {step}"):
            self._provision(c, config_path=config_path, step=step, dry_run=dry_run)

    def read_device_id(
        self, c: Optional[Context], port: str = "", baud: int = 0
    ) -> str:
        resolved_port = str(port).strip() if port else self._get_default_port()
        return self._read_device_id(c, resolved_port, baud)

    def burn_key(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        slot: str,
        key_file: Path,
        purpose: str,
        dry_run: bool = False,
    ) -> None:
        with self._stage("🔑", "BURN KEY", f"{slot} ({purpose})"):
            self._burn_key(c, port, baud, slot, key_file, purpose, dry_run)

    def protect_key(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        slot: str,
        read_protect: bool,
        write_protect: bool,
        dry_run: bool = False,
    ) -> None:
        with self._stage(
            "🔒", "LOCK KEY", f"{slot} [R:{read_protect}, W:{write_protect}]"
        ):
            self._protect_key(c, port, baud, slot, read_protect, write_protect, dry_run)

    def burn_register(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        register_name: str,
        value: str,
        dry_run: bool = False,
    ) -> None:
        with self._stage("🛡️", "BURN REGISTER", f"{register_name} = {value}"):
            self._burn_register(c, port, baud, register_name, value, dry_run)

    def flash_layout(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        chip: str,
        flash_mode: str,
        flash_freq: str,
        flash_size: str,
        flash_args: List[str],
        dry_run: bool = False,
    ) -> None:
        with self._stage("⚡", "FLASH LAYOUT", f"{chip} ({flash_size})"):
            self._flash_layout(
                c,
                port,
                baud,
                chip,
                flash_mode,
                flash_freq,
                flash_size,
                flash_args,
                dry_run,
            )

    def wait_for_device_connection(
        self,
        c: Optional[Context],
        port: str = "",
        baud: int = 0,
        timeout_sec: float = 60.0,
    ) -> str:
        resolved_port = str(port).strip() if port else self._get_default_port()
        return self._wait_for_device_connection(c, resolved_port, baud, timeout_sec)

    def wait_for_device_disconnection(
        self,
        c: Optional[Context],
        port: str = "",
        check_interval_sec: float = 0.5,
    ) -> None:
        resolved_port = str(port).strip() if port else self._get_default_port()
        self._wait_for_device_disconnection(c, resolved_port, check_interval_sec)

    # --- ABSTRACT HOOKS ---

    @abstractmethod
    def _build(
        self,
        c: Optional[Context],
        target: str,
        image_bin: str,
        dry_run: bool,
        opts: str,
    ) -> None:
        pass

    @abstractmethod
    def _flash(self, c: Optional[Context], port: str, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _monitor(
        self, c: Optional[Context], port: str, dry_run: bool, opts: str
    ) -> None:
        pass

    @abstractmethod
    def _test(self, c: Optional[Context], dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _clean(self, c: Optional[Context], dry_run: bool) -> None:
        pass

    @abstractmethod
    def _clean_all(self, c: Optional[Context], dry_run: bool) -> None:
        pass

    @abstractmethod
    def _configure(self, c: Optional[Context], dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _erase(self, c: Optional[Context], port: str, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _provision(
        self,
        c: Optional[Context],
        config_path: str,
        step: str,
        dry_run: bool,
    ) -> None:
        pass

    @abstractmethod
    def _read_device_id(self, c: Optional[Context], port: str, baud: int) -> str:
        pass

    @abstractmethod
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
        pass

    @abstractmethod
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
        pass

    @abstractmethod
    def _burn_register(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        register_name: str,
        value: str,
        dry_run: bool,
    ) -> None:
        pass

    @abstractmethod
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
        pass

    @abstractmethod
    def _wait_for_device_connection(
        self, c: Optional[Context], port: str, baud: int, timeout_sec: float
    ) -> str:
        pass

    @abstractmethod
    def _wait_for_device_disconnection(
        self, c: Optional[Context], port: str, check_interval_sec: float
    ) -> None:
        pass
