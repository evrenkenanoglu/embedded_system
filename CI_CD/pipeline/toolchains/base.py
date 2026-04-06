from abc import ABC, abstractmethod
from ..core.utils import print_stage


class BaseToolchain(ABC):
    def __init__(self, config):
        self.config = config

    # --- PUBLIC TEMPLATE METHODS (Handles printing) ---

    def build(self, *args, **kwargs):
        target = args[0] if args else kwargs.get("target", "unknown")
        print_stage(f"🔨 BUILD ({target})")
        self._build(*args, **kwargs)

    def run_unit_tests(self):
        print_stage("🧪 HOST UNIT TESTS")
        self._run_unit_tests()

    def flash_usb(self, *args, **kwargs):
        port = args[0] if args else kwargs.get("port")
        print_stage(f"⚡ FLASH USB ({port})")
        self._flash_usb(*args, **kwargs)

    def flash_ota(self, *args, **kwargs):
        port = args[0] if args else kwargs.get("port")
        print_stage(f"📡 FLASH OTA ({port})")
        self._flash_ota(*args, **kwargs)

    def clean(self):
        print_stage("🧹 CLEAN")
        self._clean()

    def clean_all(self):
        print_stage("🧹 CLEAN ALL")
        self._clean_all()

    # --- ABSTRACT METHODS (Must be implemented by subclasses) ---

    @abstractmethod
    def _build(self, *args, **kwargs):
        pass

    @abstractmethod
    def _run_unit_tests(self):
        pass

    @abstractmethod
    def _flash_usb(self, *args, **kwargs):
        pass

    @abstractmethod
    def _flash_ota(self, *args, **kwargs):
        pass

    @abstractmethod
    def _clean(self):
        pass

    @abstractmethod
    def _clean_all(self):
        pass
