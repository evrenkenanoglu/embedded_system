import subprocess
import sys
from pathlib import Path
from src.utils import find_files, get_root_dir, load_config, resolve_path


def _get_c_files_and_flags(
    config_path: Path, style_config_override: Path | None
) -> tuple[Path, list[Path], list[str]]:
    config = load_config(config_path)
    root_dir = get_root_dir(config, config_path)
    ignore_patterns = config.get("ignore_patterns", [])

    c_config = config.get("c", {})
    extensions = tuple(c_config.get("extensions", [".c", ".h", ".cpp", ".hpp"]))
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
    config_path: Path, style_config_override: Path | None = None
) -> bool:
    """Dry-run check: returns True if all C/C++ files are properly formatted."""
    root_dir, files, extra_flags = _get_c_files_and_flags(
        config_path, style_config_override
    )
    if not files:
        print(f"[C/C++] No matching files to check under {root_dir}")
        return True

    print(f"[C/C++] Checking {len(files)} file(s)...")
    all_passed = True

    for file_path in files:
        cmd = ["clang-format", "--dry-run", "--Werror"] + extra_flags + [str(file_path)]
        try:
            result = subprocess.run(
                cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )
            if result.returncode != 0:
                print(f"  [MISMATCH] {file_path.relative_to(root_dir)}")
                all_passed = False
        except FileNotFoundError:
            print("Error: 'clang-format' is not installed or not in PATH.", file=sys.stderr)
            sys.exit(1)

    if all_passed:
        print("[C/C++] All files are properly formatted.")
    else:
        print("[C/C++] Some files require formatting.")
    return all_passed


def format_c_apply(
    config_path: Path, style_config_override: Path | None = None
) -> bool:
    """In-place format: modifies C/C++ files directly."""
    root_dir, files, extra_flags = _get_c_files_and_flags(
        config_path, style_config_override
    )
    if not files:
        print(f"[C/C++] No matching files to format under {root_dir}")
        return True

    print(f"[C/C++] Formatting {len(files)} file(s)...")

    for file_path in files:
        cmd = ["clang-format", "-i"] + extra_flags + [str(file_path)]
        try:
            subprocess.run(cmd, check=True)
            print(f"  Formatted: {file_path.relative_to(root_dir)}")
        except subprocess.CalledProcessError as e:
            print(f"Error formatting {file_path}: {e}", file=sys.stderr)
            return False
        except FileNotFoundError:
            print("Error: 'clang-format' is not installed or not in PATH.", file=sys.stderr)
            sys.exit(1)

    return True