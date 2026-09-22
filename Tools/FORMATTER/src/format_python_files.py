import subprocess
import sys
from pathlib import Path
from src.utils import find_files, get_root_dir, load_config, resolve_path


def _get_python_files_and_flags(
    config_path: Path, style_config_override: Path | None
) -> tuple[Path, list[Path], list[str]]:
    config = load_config(config_path)
    root_dir = get_root_dir(config, config_path)
    ignore_patterns = config.get("ignore_patterns", [])

    py_config = config.get("python", {})
    extensions = tuple(py_config.get("extensions", [".py"]))
    files = find_files(root_dir, extensions, ignore_patterns)

    target_style = style_config_override or py_config.get("style_config")
    extra_flags: list[str] = []

    if target_style:
        style_path = resolve_path(root_dir, target_style)
        if not style_path.is_file():
            print(
                f"Error: Python style config '{style_path}' not found.",
                file=sys.stderr,
            )
            sys.exit(1)
        extra_flags.extend(["--config", str(style_path)])

    return root_dir, files, extra_flags


def format_python_check(
    config_path: Path, style_config_override: Path | None = None
) -> bool:
    """Dry-run check: returns True if all Python files are properly formatted."""
    root_dir, files, extra_flags = _get_python_files_and_flags(
        config_path, style_config_override
    )
    if not files:
        print(f"[Python] No matching files to check under {root_dir}")
        return True

    print(f"[Python] Checking {len(files)} file(s)...")
    all_passed = True

    for file_path in files:
        cmd = ["black", "--check", "-q"] + extra_flags + [str(file_path)]
        try:
            result = subprocess.run(cmd)
            if result.returncode != 0:
                print(f"  [MISMATCH] {file_path.relative_to(root_dir)}")
                all_passed = False
        except FileNotFoundError:
            print("Error: 'black' is not installed or not in PATH.", file=sys.stderr)
            sys.exit(1)

    if all_passed:
        print("[Python] All files are properly formatted.")
    else:
        print("[Python] Some files require formatting.")
    return all_passed


def format_python_apply(
    config_path: Path, style_config_override: Path | None = None
) -> bool:
    """In-place format: modifies Python files directly."""
    root_dir, files, extra_flags = _get_python_files_and_flags(
        config_path, style_config_override
    )
    if not files:
        print(f"[Python] No matching files to format under {root_dir}")
        return True

    print(f"[Python] Formatting {len(files)} file(s)...")

    for file_path in files:
        cmd = ["black", "-q"] + extra_flags + [str(file_path)]
        try:
            subprocess.run(cmd, check=True)
            print(f"  Formatted: {file_path.relative_to(root_dir)}")
        except subprocess.CalledProcessError as e:
            print(f"Error formatting {file_path}: {e}", file=sys.stderr)
            return False
        except FileNotFoundError:
            print("Error: 'black' is not installed or not in PATH.", file=sys.stderr)
            sys.exit(1)

    return True