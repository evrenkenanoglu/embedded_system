"""
@file       factory.py
@brief      Factory singleton resolver for target platform toolchains.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

from typing import Any, Dict
from core import CONFIG
from embedded_system.Tasks.toolchains.base import BaseToolchain
from embedded_system.Tasks.toolchains.esp_idf import EspIdfToolchain

_TOOLCHAIN_CACHE: Dict[str, BaseToolchain] = {}


def get_toolchain(config: Any = None) -> BaseToolchain:
    """Retrieve cached toolchain singleton based on configuration."""
    cfg = config or CONFIG
    toolchain_type = getattr(cfg, "toolchain", "esp-idf").lower()

    if toolchain_type not in _TOOLCHAIN_CACHE:
        if toolchain_type in ("esp-idf", "esp32", "espidf"):
            _TOOLCHAIN_CACHE[toolchain_type] = EspIdfToolchain(cfg)
        else:
            raise ValueError(f"Unsupported toolchain type: {toolchain_type}")

    return _TOOLCHAIN_CACHE[toolchain_type]
