"""
@file       format_c_files.py
@brief      Fast batch C/C++ formatting adapter (apply/check) using clang-format.
"""

import subprocess
import sys
from pathlib import Path
from typing import List, Optional
from src.utils import find_files, get_root_dir, load_config, resolve_path


def _get_c_files_and_flags(
    config_path: Path,
    style_config_override: Optional[Path],
    files_override: Optional[List[Path]] = None,
) -> tuple[Path, List[Path], List[str]]:
    config = load_config(config_path)
    root_dir = get_root_dir(config, config_path)
    c_config = config.get("c", {})
    extensions = tuple(c_config.get("extensions", [".c", ".h", ".cpp", ".hpp"]))

    if files_override is not None:
        target_files = [
            f
            for f in files_override
            if f.suffix in extensions and f.is_file() and f.is_relative_to(root_dir)
        ]
    else:
        ignore_patterns = config.get("ignore_patterns", [])
        target_files = find_files(root_dir, extensions, ignore_patterns)

    target_style = style_config_override or c_config.get("style_config")
    extra_flags: List[str] = []

    if target_style:
        style_path = resolve_path(root_dir, target_style)
        if not style_path.is_file():
            print(
                f"Error: Clang-format style config '{style_path}' not found.",
                file=sys.stderr,
            )
            sys.exit(1)
        extra_flags.append(f"--style=file:{style_path}")

    return root_dir, target_files, extra_flags


def format_c_check(
    config_path: Path,
    style_config_override: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """Dry-run check: verifies all target C/C++ files in a single fast process."""
    root_dir, target_files, extra_flags = _get_c_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        print(f"[C/C++] No matching files to check under {root_dir}")
        return True

    print(f"[C/C++] Checking {len(target_files)} file(s)...")

    # Single batch execution
    cmd = (
        ["clang-format", "--dry-run", "--Werror"]
        + extra_flags
        + [str(f) for f in target_files]
    )

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    except FileNotFoundError:
        print("Error: 'clang-format' is not installed or not in PATH.", file=sys.stderr)
        sys.exit(1)

    if res.returncode != 0:
        # Print formatted error output from clang-format
        for line in res.stderr.splitlines():
            if "code should be clang-formatted" in line or "error:" in line:
                print(f"  [MISMATCH] {line.strip()}")
        print(f"❌ C/C++ check failed.")
        return False

    print(f"✅ C/C++ check passed ({len(target_files)} file(s)).")
    return True


def format_c_apply(
    config_path: Path,
    style_config_override: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """In-place format: modifies all target C/C++ files in a single fast process."""
    root_dir, target_files, extra_flags = _get_c_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        print(f"[C/C++] No matching files to format under {root_dir}")
        return True

    print(f"[C/C++] Formatting {len(target_files)} file(s)...")

    # Single batch execution
    cmd = ["clang-format", "-i"] + extra_flags + [str(f) for f in target_files]

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0:
            print(f"❌ Error formatting C/C++ files:\n{res.stderr}", file=sys.stderr)
            return False
    except FileNotFoundError:
        print("Error: 'clang-format' is not installed or not in PATH.", file=sys.stderr)
        sys.exit(1)

    print(f"✅ C/C++ files formatted in-place ({len(target_files)} file(s)).")
    return True
