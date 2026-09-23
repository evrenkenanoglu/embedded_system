import subprocess
import sys
from pathlib import Path
from src.utils import find_files, get_root_dir, load_config, resolve_path


def _get_c_files_and_flags(
    config_path: Path,
    style_config_override: Path | None,
    files_override: list[Path] | None = None,
) -> tuple[Path, list[Path], list[str]]:
    config = load_config(config_path)
    root_dir = get_root_dir(config, config_path)

    c_config = config.get("c", {})
    extensions = tuple(c_config.get("extensions", [".c", ".h", ".cpp", ".hpp"]))

    if files_override is not None:
        files = [
            f
            for f in files_override
            if f.suffix in extensions and f.is_file() and f.is_relative_to(root_dir)
        ]
    else:
        ignore_patterns = config.get("ignore_patterns", [])
        files = find_files(root_dir, extensions, ignore_patterns)

    target_style = style_config_override or c_config.get("style_config")
    extra_flags: list[str] = []

    if target_style:
        style_path = resolve_path(root_dir, target_style)
        if not style_path.is_file():
            print(
                f"Error: Clang-format style config '{style_path}' not found.",
                file=sys.stderr,
            )
            sys.exit(1)
        extra_flags.append(f"--style=file:{style_path}")

    return root_dir, files, extra_flags


def format_c_check(
    config_path: Path,
    style_config_override: Path | None = None,
    files: list[Path] | None = None,
) -> bool:
    """Dry-run check: verifies all target C/C++ files in a single process."""
    root_dir, target_files, extra_flags = _get_c_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        return True

    print(f"[C/C++] Checking {len(target_files)} file(s)...")
    cmd = (
        ["clang-format", "--dry-run", "--Werror"]
        + extra_flags
        + [str(f) for f in target_files]
    )
    res = subprocess.run(cmd, capture_output=True, text=True, check=False)

    if res.returncode != 0:
        print("[C/C++] Formatting errors detected in target files.")
        return False

    print("[C/C++] All files are properly formatted.")
    return True


def format_c_apply(
    config_path: Path,
    style_config_override: Path | None = None,
    files: list[Path] | None = None,
) -> bool:
    """In-place format: modifies target C/C++ files in a single process."""
    root_dir, target_files, extra_flags = _get_c_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        return True

    print(f"[C/C++] Formatting {len(target_files)} file(s)...")
    cmd = ["clang-format", "-i"] + extra_flags + [str(f) for f in target_files]
    res = subprocess.run(cmd, check=False)
    return res.returncode == 0
