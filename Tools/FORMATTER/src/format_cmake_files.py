"""
@file       format_cmake_files.py
@brief      CMake file formatting adapter (apply/check) using cmake-format with batching
            and automatic exclusion of managed components and build artifacts.
"""

import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Tuple, Set
from src.utils import find_files, get_root_dir, load_config, resolve_path

DEFAULT_EXCLUDED_DIRS: Set[str] = {
    "build",
    "Out",
    "managed_components",
    "dependencies",
    ".venv",
    ".git",
}


def _is_cmake_file(path: Path) -> bool:
    """Identifies CMakeLists.txt or *.cmake files."""
    return path.name == "CMakeLists.txt" or path.suffix.lower() == ".cmake"


def _is_excluded(path: Path, ignore_patterns: List[str]) -> bool:
    """Checks if a path resides within excluded directory trees."""
    # 1. Check directory parts against hard exclusions
    if any(part in DEFAULT_EXCLUDED_DIRS for part in path.parts):
        return True

    # 2. Check against configuration ignore patterns
    path_str = str(path)
    for pattern in ignore_patterns:
        clean_pattern = pattern.replace("**", "").replace("*", "").strip("/\\")
        if clean_pattern and clean_pattern in path_str:
            return True

    return False


def _get_cmake_files_and_flags(
    config_path: Path,
    style_config_override: Optional[Path],
    files_override: Optional[List[Path]] = None,
) -> Tuple[Path, List[Path], List[str]]:
    config = load_config(config_path)
    root_dir = get_root_dir(config, config_path)
    cmake_config = config.get("cmake", {})
    ignore_patterns = config.get("ignore_patterns", [])

    if files_override is not None:
        files = [
            f
            for f in files_override
            if _is_cmake_file(f)
            and f.is_file()
            and f.is_relative_to(root_dir)
            and not _is_excluded(f, ignore_patterns)
        ]
    else:
        # Discover all project CMake files excluding build/managed directories
        files = []
        for p in root_dir.rglob("*"):
            if (
                p.is_file()
                and _is_cmake_file(p)
                and not _is_excluded(p, ignore_patterns)
            ):
                files.append(p)

    target_style = style_config_override or cmake_config.get("style_config")
    extra_flags: List[str] = []

    # Check for .cmake-format.yaml in root if not specified in config
    if not target_style:
        default_style = root_dir / ".cmake-format.yaml"
        if default_style.is_file():
            target_style = str(default_style)

    if target_style:
        style_path = resolve_path(root_dir, target_style)
        if not style_path.is_file():
            print(
                f"Error: CMake style config '{style_path}' not found.",
                file=sys.stderr,
            )
            sys.exit(1)
        extra_flags.append(f"--config-files={style_path}")

    return root_dir, files, extra_flags


def format_cmake_check(
    config_path: Path,
    style_config_override: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """Dry-run check: returns True if all CMake files are properly formatted."""
    root_dir, target_files, extra_flags = _get_cmake_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        print(f"[CMake] No matching files to check under {root_dir}")
        return True

    print(f"[CMake] Checking {len(target_files)} file(s)...")
    cmd = ["cmake-format", "--check"] + extra_flags + [str(f) for f in target_files]

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0:
            print("[CMake] Formatting errors detected in target files.")
            for line in res.stderr.splitlines():
                if line.strip():
                    print(f"  {line.strip()}")
            return False
    except FileNotFoundError:
        print(
            "Error: 'cmake-format' is not installed or not in PATH.",
            file=sys.stderr,
        )
        sys.exit(1)

    print("[CMake] All files are properly formatted.")
    return True


def format_cmake_apply(
    config_path: Path,
    style_config_override: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """In-place format: modifies target CMake files in a single process."""
    root_dir, target_files, extra_flags = _get_cmake_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        print(f"[CMake] No matching files to format under {root_dir}")
        return True

    print(f"[CMake] Formatting {len(target_files)} file(s)...")
    cmd = ["cmake-format", "-i"] + extra_flags + [str(f) for f in target_files]

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0:
            print(f"Error formatting CMake files:\n{res.stderr}", file=sys.stderr)
            return False
    except FileNotFoundError:
        print(
            "Error: 'cmake-format' is not installed or not in PATH.",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"✅ CMake files formatted in-place ({len(target_files)} file(s)).")
    return True
