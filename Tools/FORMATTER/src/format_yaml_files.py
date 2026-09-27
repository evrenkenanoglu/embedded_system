"""
@file       format_yaml_files.py
@brief      Fast batch YAML formatting adapter (apply/check) using yamlfix.
"""

import subprocess
import sys
from pathlib import Path
from typing import List, Optional
from src.utils import find_files, get_root_dir, load_config, resolve_path


def _get_yaml_files_and_flags(
    config_path: Path,
    style_config_override: Optional[Path],
    files_override: Optional[List[Path]] = None,
) -> tuple[Path, List[Path], List[str]]:
    config = load_config(config_path)
    root_dir = get_root_dir(config, config_path)
    yaml_config = config.get("yaml", {})
    extensions = tuple(yaml_config.get("extensions", [".yaml", ".yml"]))

    if files_override is not None:
        target_files = [
            f
            for f in files_override
            if f.suffix in extensions and f.is_file() and f.is_relative_to(root_dir)
        ]
    else:
        ignore_patterns = config.get("ignore_patterns", [])
        target_files = find_files(root_dir, extensions, ignore_patterns)

    target_style = style_config_override or yaml_config.get("style_config")
    extra_flags: List[str] = []

    if target_style:
        style_path = resolve_path(root_dir, target_style)
        if not style_path.is_file():
            print(
                f"Error: YAML style config '{style_path}' not found.",
                file=sys.stderr,
            )
            sys.exit(1)
        extra_flags.extend(["--config-path", str(style_path)])

    return root_dir, target_files, extra_flags


def format_yaml_check(
    config_path: Path,
    style_config_override: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """Dry-run check: verifies all target YAML files in a single fast process."""
    root_dir, target_files, extra_flags = _get_yaml_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        print(f"[YAML] No matching files to check under {root_dir}")
        return True

    print(f"[YAML] Checking {len(target_files)} file(s)...")

    # Single batch check execution
    cmd = ["yamlfix", "--check"] + extra_flags + [str(f) for f in target_files]

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    except FileNotFoundError:
        print("Error: 'yamlfix' is not installed or not in PATH.", file=sys.stderr)
        sys.exit(1)

    if res.returncode != 0:
        for line in (res.stderr + res.stdout).splitlines():
            if any(k in line.lower() for k in ("fixed", "would be", "failed")):
                print(f"  [MISMATCH] {line.strip()}")
        print(f"❌ YAML check failed.")
        return False

    print(f"✅ YAML check passed ({len(target_files)} file(s)).")
    return True


def format_yaml_apply(
    config_path: Path,
    style_config_override: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """In-place format: modifies all target YAML files in a single fast process."""
    root_dir, target_files, extra_flags = _get_yaml_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        print(f"[YAML] No matching files to format under {root_dir}")
        return True

    print(f"[YAML] Formatting {len(target_files)} file(s)...")

    # Single batch format execution
    cmd = ["yamlfix"] + extra_flags + [str(f) for f in target_files]

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0:
            print(f"❌ Error formatting YAML files:\n{res.stderr}", file=sys.stderr)
            return False
    except FileNotFoundError:
        print(
            "Error: 'yamlfix' is not installed or not in PATH.",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"✅ YAML files formatted in-place ({len(target_files)} file(s)).")
    return True
