from abc import ABC, abstractmethod
from ..core.utils import print_stage

class BaseToolchain(ABC):
    def __init__(self, config):
        self.config = config

    # --- PUBLIC TEMPLATE METHODS (Handles printing) ---

    def build(self, target):
        print_stage(f"🔨 BUILD ({target})")
        self._build(target)

    def run_unit_tests(self):
        print_stage("🧪 HOST UNIT TESTS")
        self._run_unit_tests()

    def flash_usb(self, port, binary_dir):
        print_stage(f"⚡ FLASH USB ({port})")
        self._flash_usb(port, binary_dir)

    def flash_ota(self, port, binary_dir, ota_port):
        print_stage(f"📡 FLASH OTA ({port})")
        self._flash_ota(port, binary_dir, ota_port)

    # --- ABSTRACT METHODS (Must be implemented by subclasses) ---

    @abstractmethod
    def _build(self, target): 
        pass

    @abstractmethod
    def _run_unit_tests(self): 
        pass

    @abstractmethod
    def _flash_usb(self, port, binary_dir): 
        pass

    @abstractmethod
    def _flash_ota(self, port, binary_dir, ota_port): 
        pass