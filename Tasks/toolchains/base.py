import time
from abc import abstractmethod
from contextlib import contextmanager
from typing import Any, Generator
from invoke import Context
from invoke.exceptions import UnexpectedExit

from embedded_system.Tasks.toolchains.interface import IToolchain


class BaseToolchain(IToolchain):
    def __init__(self, config: Any) -> None:
        self.config = config

    @contextmanager
    def _stage(self, emoji: str, action: str, details: str = "") -> Generator[None, None, None]:
        CYAN = "\033[1;36m"
        GREEN = "\033[1;32m"
        RED = "\033[1;31m"
        WHITE_BOLD = "\033[1;37m"
        RESET = "\033[0m"

        target_info = f" ({details})" if details else ""
        header = f"{emoji} {action.upper()}{target_info}"
        width = 70
        line = "━" * width

        print(f"\n{CYAN}{line}{RESET}")
        print(f"{CYAN} ❯ {WHITE_BOLD}{header}{RESET}")
        print(f"{CYAN}{line}{RESET}\n")

        start_time = time.perf_counter()
        try:
            yield
            elapsed = time.perf_counter() - start_time
            print(f"\n{GREEN}✅ {action.upper()} COMPLETED {RESET}[{elapsed:.2f}s]\n")
        except UnexpectedExit as err:
            elapsed = time.perf_counter() - start_time
            exit_code = err.result.returncode if err.result else "unknown"
            print(f"\n{RED}❌ {action.upper()} FAILED {RESET}(Exit code {exit_code}) [{elapsed:.2f}s]\n")
            raise
        except Exception as err:
            elapsed = time.perf_counter() - start_time
            print(f"\n{RED}❌ {action.upper()} FAILED {RESET}[{elapsed:.2f}s]: {err}\n")
            raise

    def build(self, c: Context, target: str = "esp32", image_bin: str = "", dry_run: bool = False, opts: str = "") -> None:
        with self._stage("🔨", "BUILD", target):
            self._build(c, target=target, image_bin=image_bin, dry_run=dry_run, opts=opts)

    def flash(self, c: Context, port: str = "", dry_run: bool = False, opts: str = "") -> None:
        port_label = port if port else "AUTO"
        with self._stage("⚡", "FLASH USB", port_label):
            self._flash(c, port=port, dry_run=dry_run, opts=opts)

    def flash_ota(self, c: Context, port: str = "", ota_port: int = 8032, dry_run: bool = False, opts: str = "") -> None:
        port_label = port if port else "AUTO"
        with self._stage("📡", "FLASH OTA", f"{port_label}:{ota_port}"):
            self._flash_ota(c, port=port, ota_port=ota_port, dry_run=dry_run, opts=opts)

    def monitor(self, c: Context, port: str = "", dry_run: bool = False, opts: str = "") -> None:
        port_label = port if port else "AUTO"
        with self._stage("🖥️", "MONITOR", port_label):
            self._monitor(c, port=port, dry_run=dry_run, opts=opts)

    def erase(self, c: Context, port: str = "", dry_run: bool = False, opts: str = "") -> None:
        port_label = port if port else "AUTO"
        with self._stage("🗑️", "ERASE FLASH", port_label):
            self._erase(c, port=port, dry_run=dry_run, opts=opts)

    def menuconfig(self, c: Context, dry_run: bool = False, opts: str = "") -> None:
        with self._stage("⚙️", "MENUCONFIG"):
            self._menuconfig(c, dry_run=dry_run, opts=opts)

    def test(self, c: Context, dry_run: bool = False, opts: str = "") -> None:
        with self._stage("🧪", "HOST UNIT TESTS"):
            self._test(c, dry_run=dry_run, opts=opts)

    def clean(self, c: Context, dry_run: bool = False) -> None:
        with self._stage("🧹", "CLEAN"):
            self._clean(c, dry_run=dry_run)

    def clean_all(self, c: Context, dry_run: bool = False) -> None:
        with self._stage("🧹", "FULL CLEAN"):
            self._clean_all(c, dry_run=dry_run)

    @abstractmethod
    def _build(self, c: Context, target: str, image_bin: str, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _flash(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _flash_ota(self, c: Context, port: str, ota_port: int, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _monitor(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _erase(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _menuconfig(self, c: Context, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _test(self, c: Context, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _clean(self, c: Context, dry_run: bool) -> None:
        pass

    @abstractmethod
    def _clean_all(self, c: Context, dry_run: bool) -> None:
        pass