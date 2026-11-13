"""
@file       render.py
@brief      Payload-agnostic QR symbol construction and PNG/SVG output.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

from pathlib import Path

from qr_engine import QrToolError

ECC_LEVELS = ("L", "M", "Q", "H")

# Byte-mode data capacity of QR version 40 per error-correction level (ISO/IEC 18004).
BYTE_CAPACITY = {"L": 2953, "M": 2331, "Q": 1663, "H": 1273}


def _import_segno():
    try:
        import segno
    except ImportError as exc:
        raise QrToolError(
            "the 'segno' library is not installed (requires: pip install segno)"
        ) from exc
    return segno


def build_qr(payload: str, ecc: str):
    """Encodes the payload at exactly the requested ECC level (no silent boosting)."""
    segno = _import_segno()
    try:
        return segno.make(payload, error=ecc, micro=False, boost_error=False)
    except segno.DataOverflowError as exc:
        size = len(payload.encode("utf-8"))
        raise QrToolError(
            f"payload is {size} bytes, which exceeds QR capacity at ECC level {ecc} "
            f"(byte-mode limit {BYTE_CAPACITY[ecc]} bytes, version 40). "
            "Shorten the payload or use a lower error-correction level."
        ) from exc


def save_symbol(
    qr, path: Path, fmt: str, scale: int, border: int, overwrite: bool
) -> None:
    """Writes the symbol as 'png' or 'svg'. Existing files are kept unless overwrite is set."""
    mode = "wb" if overwrite else "xb"
    try:
        with open(path, mode) as fh:
            qr.save(fh, kind=fmt, scale=scale, border=border)
    except FileExistsError as exc:
        raise QrToolError(
            f"output already exists: {path} (use --force to overwrite)"
        ) from exc
    except BaseException:
        path.unlink(missing_ok=True)
        raise
