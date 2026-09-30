"""
@file       factory.py
@brief      Factory singleton resolver for target platform toolchains.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

from typing import Any, Dict
from embedded_system.Tasks.toolchains.base import BaseToolchain
from embedded_system.Tasks.toolchains.esp_idf import EspIdfToolchain

_TOOLCHAIN_CACHE: Dict[str, BaseToolchain] = {}


def get_toolchain(config: Any = None) -> BaseToolchain:
    """Retrieve cached toolchain singleton based on configuration."""
    cfg = config
    if cfg is None:
        try:
            from core import CONFIG
            cfg = CONFIG
        except ImportError:
            raise ValueError("No configuration provided and 'core.CONFIG' could not be resolved.")

    # Support dictionary-based or object-based config schemas
    if isinstance(cfg, dict):
        toolchain_type = cfg.get("toolchain") or cfg.get("target_platform") or cfg.get("target", {}).get("chip", "esp-idf")
    else:
        toolchain_type = getattr(cfg, "toolchain", None) or getattr(cfg, "target_platform", "esp-idf")

    toolchain_type = str(toolchain_type).lower()

    if toolchain_type not in _TOOLCHAIN_CACHE:
        if toolchain_type in ("esp-idf", "esp32", "esp32s3", "espidf"):
            _TOOLCHAIN_CACHE[toolchain_type] = EspIdfToolchain(cfg)
        else:
            raise ValueError(f"Unsupported toolchain type: {toolchain_type}")

    return _TOOLCHAIN_CACHE[toolchain_type]
