import subprocess
import sys
from pathlib import Path
from src.utils import find_files, get_root_dir, load_config, resolve_path


def _get_python_files_and_flags(
    config_path: Path,
    style_config_override: Path | None,
    files_override: list[Path] | None = None,
) -> tuple[Path, list[Path], list[str]]:
    config = load_config(config_path)
    root_dir = get_root_dir(config, config_path)

    py_config = config.get("python", {})
    extensions = tuple(py_config.get("extensions", [".py"]))

    if files_override is not None:
        files = [
            f
            for f in files_override
            if f.suffix in extensions and f.is_file() and f.is_relative_to(root_dir)
        ]
    else:
        ignore_patterns = config.get("ignore_patterns", [])
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
        extra_flags.append(f"--config={style_path}")

    return root_dir, files, extra_flags


def format_python_check(
    config_path: Path,
    style_config_override: Path | None = None,
    files: list[Path] | None = None,
) -> bool:
    """Dry-run check: verifies all target Python files in a single process."""
    root_dir, target_files, extra_flags = _get_python_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        return True

    print(f"[Python] Checking {len(target_files)} file(s)...")
    config = load_config(config_path)
    tool = config.get("python", {}).get("tool", "black")

    cmd = [tool, "--check", "--quiet"] + extra_flags + [str(f) for f in target_files]
    res = subprocess.run(cmd, check=False)
    return res.returncode == 0


def format_python_apply(
    config_path: Path,
    style_config_override: Path | None = None,
    files: list[Path] | None = None,
) -> bool:
    """In-place format: modifies target Python files in a single process."""
    root_dir, target_files, extra_flags = _get_python_files_and_flags(
        config_path, style_config_override, files_override=files
    )
    if not target_files:
        return True

    print(f"[Python] Formatting {len(target_files)} file(s)...")
    config = load_config(config_path)
    tool = config.get("python", {}).get("tool", "black")

    cmd = [tool, "--quiet"] + extra_flags + [str(f) for f in target_files]
    res = subprocess.run(cmd, check=False)
    return res.returncode == 0
