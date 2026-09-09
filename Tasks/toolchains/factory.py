from functools import lru_cache
from typing import Any
from core import CONFIG
from embedded_system.Tasks.toolchains.base import BaseToolchain
from embedded_system.Tasks.toolchains.esp_idf import EspIdfToolchain


@lru_cache(maxsize=2)
def get_toolchain(config: Any = None) -> BaseToolchain:
    """Retrieve cached toolchain singleton based on configuration."""
    cfg = config or CONFIG
    toolchain_type = getattr(cfg, "toolchain", "esp-idf").lower()

    if toolchain_type in ("esp-idf", "esp32", "espidf"):
        return EspIdfToolchain(cfg)

    raise ValueError(f"Unsupported toolchain type: {toolchain_type}")