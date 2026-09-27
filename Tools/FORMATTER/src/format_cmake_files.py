"""
@file       format_cmake_files.py
@brief      Fast batch CMake formatting adapter (apply/check) using cmake-format.
"""

import subprocess
import sys
from pathlib import Path
from typing import List, Optional
from src.utils import find_files, get_root_dir, load_config, resolve_path


def _get_cmake_files_and_flags(
    config_path: Path,
    style_config_override: Optional[Path],
    files_override: Optional[List[Path]] = None,
) -> tuple[Path, List[Path], List[str]]:
    config = load_config(config_path)
    root_dir = get_root_dir(config, config_path)
    cmake_config = config.get("cmake", {})
    extensions = tuple(cmake_config.get("extensions", [".cmake"]))

    if files_override is not None:
        target_files = [
            f
            for f in files_override
            if (f.suffix in extensions or f.name == "CMakeLists.txt")
            and f.is_file()
            and f.is_relative_to(root_dir)
        ]
    else:
        ignore_patterns = config.get("ignore_patterns", [])
        # Search for both .cmake extension and CMakeLists.txt files
        all_found = find_files(root_dir, extensions + (".txt",), ignore_patterns)
        target_files = [
            f for f in all_found if f.suffix in extensions or f.name == "CMakeLists.txt"
        ]

    target_style = style_config_override or cmake_config.get("style_config")
    extra_flags: List[str] = []

    if target_style:
        style_path = resolve_path(root_dir, target_style)
        if not style_path.is_file():
            print(
                f"Error: CMake style config '{style_path}' not found.",
                file=sys.stderr,
            )
            sys.exit(1)
        extra_flags.extend(["-c", str(style_path)])

    return root_dir, target_files, extra_flags


def format_cmake_check(
    config_path: Path,
    style_config_override: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """Dry-run check: verifies all target CMake files in a single fast process."""
    root_dir, target_files, extra_flags = _get_cmake_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        print(f"[CMake] No matching files to check under {root_dir}")
        return True

    print(f"[CMake] Checking {len(target_files)} file(s)...")

    # Single batch check execution using cmake-format
    cmd = ["cmake-format", "--check"] + extra_flags + [str(f) for f in target_files]

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    except FileNotFoundError:
        print(
            "Error: 'cmake-format' is not installed or not in PATH.",
            file=sys.stderr,
        )
        sys.exit(1)

    if res.returncode != 0:
        for line in (res.stderr + res.stdout).splitlines():
            if (
                "format" in line.lower()
                or "error" in line.lower()
                or "diff" in line.lower()
            ):
                print(f"  [MISMATCH] {line.strip()}")
        print(f"❌ CMake check failed.")
        return False

    print(f"✅ CMake check passed ({len(target_files)} file(s)).")
    return True


def format_cmake_apply(
    config_path: Path,
    style_config_override: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """In-place format: modifies all target CMake files in a single fast process."""
    root_dir, target_files, extra_flags = _get_cmake_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        print(f"[CMake] No matching files to format under {root_dir}")
        return True

    print(f"[CMake] Formatting {len(target_files)} file(s)...")

    # Single batch format execution using cmake-format
    cmd = ["cmake-format", "-i"] + extra_flags + [str(f) for f in target_files]

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0:
            print(f"❌ Error formatting CMake files:\n{res.stderr}", file=sys.stderr)
            return False
    except FileNotFoundError:
        print(
            "Error: 'cmake-format' is not installed or not in PATH.",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"✅ CMake files formatted in-place ({len(target_files)} file(s)).")
    return True
