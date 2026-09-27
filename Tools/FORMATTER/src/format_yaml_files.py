"""
@file       format_yaml_files.py
@brief      Fast batch YAML formatting adapter (apply/check) with strict syntax & duplicate key diagnostics.
"""

import subprocess
import sys
from pathlib import Path
from typing import List, Optional
import yaml

from src.utils import find_files, get_root_dir, load_config, resolve_path


class _StrictSafeLoader(yaml.SafeLoader):
    """Custom YAML loader that rejects duplicate keys."""

    pass


def _construct_mapping(loader, node, deep=False):
    loader.flatten_mapping(node)
    mapping = {}
    for key_node, value_node in node.value:
        key = loader.construct_object(key_node, deep=deep)
        if key in mapping:
            raise yaml.constructor.ConstructorError(
                "while constructing a mapping",
                node.start_mark,
                f"found duplicate key '{key}'",
                key_node.start_mark,
            )
        mapping[key] = loader.construct_object(value_node, deep=deep)
    return mapping


_StrictSafeLoader.add_constructor(
    yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, _construct_mapping
)


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
                f"Error: YAML style config '{style_path}' not found.", file=sys.stderr
            )
            sys.exit(1)
        extra_flags.extend(["--config-path", str(style_path)])

    return root_dir, target_files, extra_flags


def _validate_yaml_syntax(target_files: List[Path], root_dir: Path) -> bool:
    """Pre-checks all YAML files for syntax errors and duplicate keys."""
    has_errors = False
    for file_path in target_files:
        try:
            with open(file_path, "r", encoding="utf-8") as f:
                yaml.load(f, Loader=_StrictSafeLoader)
        except yaml.YAMLError as exc:
            has_errors = True
            rel_file = file_path.relative_to(root_dir)
            print(f"  [YAML SYNTAX ERROR] {rel_file}:")
            if hasattr(exc, "problem_mark") and exc.problem_mark:
                mark = exc.problem_mark
                print(
                    f"    Line {mark.line + 1}, Column {mark.column + 1}: {exc.problem}"
                )
            else:
                print(f"    {exc}")
    return not has_errors


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

    # Strict pre-validation
    if not _validate_yaml_syntax(target_files, root_dir):
        print("❌ YAML check failed (syntax or duplicate key errors).")
        return False

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
        print("❌ YAML check failed.")
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

    # Strict pre-validation
    if not _validate_yaml_syntax(target_files, root_dir):
        print("❌ YAML formatting aborted due to syntax or duplicate key errors.")
        return False

    cmd = ["yamlfix"] + extra_flags + [str(f) for f in target_files]

    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0:
            print(f"❌ Error formatting YAML files:\n{res.stderr}", file=sys.stderr)
            return False
    except FileNotFoundError:
        print("Error: 'yamlfix' is not installed or not in PATH.", file=sys.stderr)
        sys.exit(1)

    print(f"✅ YAML files formatted in-place ({len(target_files)} file(s)).")
    return True
