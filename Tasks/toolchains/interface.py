from abc import ABC, abstractmethod
from typing import Optional
from invoke import Context


class IToolchain(ABC):
    """Universal interface common to all embedded platforms."""

    @abstractmethod
    def build(
        self,
        c: Context,
        target: str = "",
        image_bin: str = "",
        dry_run: bool = False,
        opts: str = "",
    ) -> None:
        """Compile firmware and generate output binaries."""
        pass

    @abstractmethod
    def flash(
        self, c: Context, port: str = "", dry_run: bool = False, opts: str = ""
    ) -> None:
        """Flash firmware onto target device."""
        pass

    @abstractmethod
    def monitor(
        self, c: Context, port: str = "", dry_run: bool = False, opts: str = ""
    ) -> None:
        """Open interactive device monitor / serial console."""
        pass

    @abstractmethod
    def test(self, c: Context, dry_run: bool = False, opts: str = "") -> None:
        """Run host-based unit tests / emulation."""
        pass

    @abstractmethod
    def clean(self, c: Context, dry_run: bool = False) -> None:
        """Clean intermediate build files."""
        pass

    @abstractmethod
    def clean_all(self, c: Context, dry_run: bool = False) -> None:
        """Purge entire build output and configuration cache."""
        pass
