"""
@file       __init__.py
@brief      Shared constants and error type for the generic QR generator engine.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

TOOL_NAME = "qr-generator"
TOOL_VERSION = "1.0.0"


class QrToolError(Exception):
    """User-facing failure. Messages must never contain the payload."""
