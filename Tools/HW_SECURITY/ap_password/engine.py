"""
@file       engine.py
@brief      Per-device SoftAP passphrase generation and provisioning record writer.
@note       Standard library only. Independent of the task-runner core and the project SSoT.
"""

import json
import os
import re
import secrets
import stat
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict

MIN_LENGTH = 8
MAX_LENGTH = 63
DEFAULT_LENGTH = 16
SCHEMA_VERSION = 1

# WPA2/WPA3 passphrases are printable ASCII. Visually ambiguous glyphs (0/O, 1/l/I, o) are excluded.
PASSWORD_ALPHABET = "ABCDEFGHJKMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789"
AMBIGUOUS_GLYPHS = frozenset("0O1lIo")

_DEVICE_ID_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_-]{0,63}$")


def validate_device_id(device_id: str) -> str:
    if not _DEVICE_ID_PATTERN.fullmatch(device_id or ""):
        raise ValueError(
            "device_id must be 1..64 characters of letters, digits, '-' or '_', starting with a letter or digit"
        )
    return device_id


def generate_password(length: int = DEFAULT_LENGTH) -> str:
    """Returns a CSPRNG-generated passphrase containing upper, lower and digit characters."""
    if not MIN_LENGTH <= length <= MAX_LENGTH:
        raise ValueError(
            f"password length must be in the range {MIN_LENGTH}..{MAX_LENGTH}"
        )
    while True:
        candidate = "".join(secrets.choice(PASSWORD_ALPHABET) for _ in range(length))
        if (
            any(c.isupper() for c in candidate)
            and any(c.islower() for c in candidate)
            and any(c.isdigit() for c in candidate)
        ):
            return candidate


def build_record(device_id: str, length: int = DEFAULT_LENGTH) -> Dict[str, Any]:
    return {
        "schema_version": SCHEMA_VERSION,
        "device_id": validate_device_id(device_id),
        "ap_password": generate_password(length),
        "created_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    }


def write_record(output: Path, record: Dict[str, Any], force: bool = False) -> Path:
    """Writes the record with owner-only permissions. Refuses to overwrite unless force is set."""
    output = output.expanduser().resolve()
    output.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    parent_metadata = output.parent.stat()
    if stat.S_IMODE(parent_metadata.st_mode) & 0o077 or (
        hasattr(os, "geteuid") and parent_metadata.st_uid != os.geteuid()
    ):
        raise PermissionError(
            "AP password record directory must be private and user-owned"
        )
    flags = os.O_WRONLY | os.O_CREAT | (os.O_TRUNC if force else os.O_EXCL)
    try:
        fd = os.open(output, flags, 0o600)
    except FileExistsError as err:
        raise FileExistsError(
            f"refusing to overwrite existing record: {output} (use --force)"
        ) from err
    with os.fdopen(fd, "w", encoding="utf-8") as handle:
        json.dump(record, handle, indent=2)
        handle.write("\n")
    os.chmod(output, 0o600)
    return output


def load_password_record(record_path: Path, expected_device_id: str) -> str:
    """Loads a private provisioning record and validates its device binding."""
    path = record_path.expanduser()
    if path.is_symlink():
        raise ValueError("AP password record must not be a symbolic link")
    path = path.resolve()
    metadata = path.stat()
    if not stat.S_ISREG(metadata.st_mode) or stat.S_IMODE(metadata.st_mode) & 0o077:
        raise ValueError("AP password record must be a regular owner-only file")
    if hasattr(os, "geteuid") and metadata.st_uid != os.geteuid():
        raise ValueError("AP password record must be owned by the current user")

    with path.open("r", encoding="utf-8") as source:
        record = json.load(source)
    if (
        not isinstance(record, dict)
        or type(record.get("schema_version")) is not int
        or record.get("schema_version") != SCHEMA_VERSION
    ):
        raise ValueError("AP password record schema version is unsupported")
    if record.get("device_id") != expected_device_id:
        raise ValueError(
            "AP password record device ID does not match provisioning config"
        )

    password = record.get("ap_password")
    if (
        not isinstance(password, str)
        or not MIN_LENGTH <= len(password) <= MAX_LENGTH
        or not password.isascii()
        or any(char not in PASSWORD_ALPHABET for char in password)
        or not any(char.isupper() for char in password)
        or not any(char.islower() for char in password)
        or not any(char.isdigit() for char in password)
    ):
        raise ValueError("AP password record contains an invalid passphrase")
    return password


def write_password_file(path: Path, password: str) -> Path:
    """Writes a passphrase without a newline to a new owner-only temporary file."""
    path = path.resolve()
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w", encoding="ascii") as output:
        output.write(password)
    os.chmod(path, 0o600)
    return path
