import os
import sys
import time
from abc import abstractmethod
from contextlib import contextmanager
from typing import Any, Generator, Optional
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
    def _stage(self, emoji: str, action: str, details: str = "") -> Generator[None, None, None]:
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

    def build(self, c: Context, target: str = "", image_bin: str = "", dry_run: bool = False, opts: str = "") -> None:
        resolved_target = str(target).strip() if target and str(target).strip() else self._get_default_target()
        with self._stage("🔨", "BUILD", resolved_target):
            self._build(c, target=resolved_target, image_bin=image_bin, dry_run=dry_run, opts=opts)

    def flash(self, c: Context, port: str = "", dry_run: bool = False, opts: str = "") -> None:
        resolved_port = str(port).strip() if port and str(port).strip() else self._get_default_port()
        port_label = resolved_port or "AUTO"
        with self._stage("⚡", "FLASH", port_label):
            self._flash(c, port=resolved_port, dry_run=dry_run, opts=opts)

    def monitor(self, c: Context, port: str = "", dry_run: bool = False, opts: str = "") -> None:
        resolved_port = str(port).strip() if port and str(port).strip() else self._get_default_port()
        port_label = resolved_port or "AUTO"
        with self._stage("🖥️", "MONITOR", port_label):
            self._monitor(c, port=resolved_port, dry_run=dry_run, opts=opts)

    def test(self, c: Context, dry_run: bool = False, opts: str = "") -> None:
        with self._stage("🧪", "HOST UNIT TESTS"):
            self._test(c, dry_run=dry_run, opts=opts)

    def clean(self, c: Context, dry_run: bool = False) -> None:
        with self._stage("🧹", "CLEAN"):
            self._clean(c, dry_run=dry_run)

    def clean_all(self, c: Context, dry_run: bool = False) -> None:
        with self._stage("🧹", "FULL CLEAN"):
            self._clean_all(c, dry_run=dry_run)

    # --- ABSTRACT HOOKS ---

    @abstractmethod
    def _build(self, c: Context, target: str, image_bin: str, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _flash(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
        pass

    @abstractmethod
    def _monitor(self, c: Context, port: str, dry_run: bool, opts: str) -> None:
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