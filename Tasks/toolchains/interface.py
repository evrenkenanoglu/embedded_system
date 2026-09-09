from abc import ABC, abstractmethod
from invoke import Context


class IToolchain(ABC):
    @abstractmethod
    def build(self, c: Context, target: str = "esp32", image_bin: str = "", dry_run: bool = False, opts: str = "") -> None:
        pass

    @abstractmethod
    def flash(self, c: Context, port: str = "", dry_run: bool = False, opts: str = "") -> None:
        pass

    @abstractmethod
    def flash_ota(self, c: Context, port: str = "", ota_port: int = 8032, dry_run: bool = False, opts: str = "") -> None:
        pass

    @abstractmethod
    def monitor(self, c: Context, port: str = "", dry_run: bool = False, opts: str = "") -> None:
        pass

    @abstractmethod
    def erase(self, c: Context, port: str = "", dry_run: bool = False, opts: str = "") -> None:
        pass

    @abstractmethod
    def menuconfig(self, c: Context, dry_run: bool = False, opts: str = "") -> None:
        pass

    @abstractmethod
    def test(self, c: Context, dry_run: bool = False, opts: str = "") -> None:
        pass

    @abstractmethod
    def clean(self, c: Context, dry_run: bool = False) -> None:
        pass

    @abstractmethod
    def clean_all(self, c: Context, dry_run: bool = False) -> None:
        pass