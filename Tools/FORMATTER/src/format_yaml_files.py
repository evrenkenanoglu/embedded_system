"""
@file       format_yaml_files.py
@brief      YAML formatting adapter (apply/check) supporting git-modified file filtering.
"""

import subprocess
import sys
from pathlib import Path
from typing import List, Optional
import yaml


def _get_target_directories(config_path: Path) -> List[str]:
    """Extracts scan directories from configuration or falls back to standard roots."""
    targets = ["configs", "CI_CD", ".github"]
    if config_path.exists():
        try:
            with open(config_path, "r", encoding="utf-8") as f:
                data = yaml.safe_load(f) or {}
            configs_dir = data.get("paths", {}).get("configs_dir")
            if configs_dir and Path(configs_dir).exists():
                targets = [configs_dir, "CI_CD", ".github"]
        except Exception:
            pass
    return [t for t in targets if Path(t).exists()]


def _filter_yaml_files(files: Optional[List[Path]], config_path: Path) -> List[str]:
    """Filters target file list for YAML files or returns target directories."""
    if files is not None:
        return [
            str(f) for f in files if str(f).endswith((".yaml", ".yml")) and f.exists()
        ]
    return _get_target_directories(config_path)


def format_yaml_check(
    config_path: Path,
    style_config: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """Validates YAML files and outputs only filenames that require formatting."""
    targets = _filter_yaml_files(files, config_path)
    if files is not None and not targets:
        return True

    cmd = ["yamlfix", "--check"] + targets
    res = subprocess.run(cmd, capture_output=True, text=True, check=False)

    if res.returncode != 0:
        for line in res.stderr.splitlines() + res.stdout.splitlines():
            if (
                "fixed" in line.lower()
                or "would be" in line.lower()
                or "failed" in line.lower()
            ):
                print(line.strip())
        return False

    print(f"✅ YAML check passed ({len(targets)} targets).")
    return True


def format_yaml_apply(
    config_path: Path,
    style_config_override: Optional[Path] = None,
    files: Optional[List[Path]] = None,
) -> bool:
    """In-place format: modifies target YAML files in a single process."""
    targets = _filter_yaml_files(files, config_path)
    if not targets:
        root_dir = (
            get_root_dir(load_config(config_path), config_path)
            if config_path.exists()
            else config_path.parent
        )
        print(f"[YAML] No matching files to format under {root_dir}")
        return True

    print(f"[YAML] Formatting {len(targets)} file(s)...")
    cmd = ["yamlfix"] + [str(t) for t in targets]

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

    print(f"✅ YAML files formatted in-place ({len(targets)} file(s)).")
    return True
