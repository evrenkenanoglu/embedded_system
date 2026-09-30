from abc import ABC, abstractmethod
from pathlib import Path
from typing import Dict, List, Optional
from invoke import Context


class IToolchain(ABC):
    """Universal interface common to all embedded platforms."""

    # 1. Compilation & Cleanup
    @abstractmethod
    def build(
        self,
        c: Optional[Context],
        target: str = "",
        image_bin: str = "",
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        pass

    @abstractmethod
    def clean(self, c: Optional[Context], dry_run: bool = False) -> None:
        pass

    @abstractmethod
    def clean_all(self, c: Optional[Context], dry_run: bool = False) -> None:
        pass

    @abstractmethod
    def configure(
        self, c: Optional[Context], dry_run: bool = False, opts: str = ""
    ) -> None:
        pass

    @abstractmethod
    def test(self, c: Optional[Context], dry_run: bool = False, opts: str = "") -> None:
        pass

    # 2. Flashing & Device Monitor
    @abstractmethod
    def flash(
        self,
        c: Optional[Context],
        port: str = "",
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        pass

    @abstractmethod
    def erase(
        self,
        c: Optional[Context],
        port: str = "",
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        pass

    @abstractmethod
    def monitor(
        self,
        c: Optional[Context],
        port: str = "",
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        pass

    # 3. Hardware Silicon Provisioning
    @abstractmethod
    def read_device_id(
        self, c: Optional[Context], port: str = "", baud: int = 0
    ) -> str:
        """Queries unique silicon identifier (MAC or hardware UUID)."""
        pass

    @abstractmethod
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
        """Programs a cryptographic key into hardware storage/eFuse."""
        pass

    @abstractmethod
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
        """Applies permanent hardware read and write protection to a key slot."""
        pass

    @abstractmethod
    def burn_register(
        self,
        c: Optional[Context],
        port: str,
        baud: int,
        register_name: str,
        value: str,
        dry_run: bool = False,
    ) -> None:
        """Burns hardware configuration register or security lock bit."""
        pass

    @abstractmethod
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
        """Flashes binary partition map to physical storage."""
        pass

    @abstractmethod
    def wait_for_device_connection(
        self, c: Optional[Context], port: str, baud: int, timeout_sec: float = 60.0
    ) -> str:
        """Polls until a target device connects and returns its hardware ID."""
        pass

    @abstractmethod
    def wait_for_device_disconnection(
        self, c: Optional[Context], port: str, check_interval_sec: float = 0.5
    ) -> None:
        """Waits until the device is disconnected from the fixture."""
        pass

    @abstractmethod
    def enable_virtual_mode(self, virtual_state_file: Path) -> None:
        """Enables platform-independent simulation of hardware eFuses and registers."""
        pass

    @abstractmethod
    def is_virtual_mode(self) -> bool:
        """Returns True if the toolchain is targeting a software simulation model."""
        pass
