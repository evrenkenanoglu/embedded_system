import argparse
import inspect
import subprocess
import sys
from pathlib import Path
from typing import List, Optional

# Add project directory to Python module search path
sys.path.insert(0, str(Path(__file__).resolve().parent))

from src.format_c_files import format_c_apply, format_c_check
from src.format_python_files import format_python_apply, format_python_check
from src.format_yaml_files import format_yaml_apply, format_yaml_check


def get_git_modified_files() -> List[Path]:
    """Retrieves all tracked and untracked uncommitted files from git status."""
    cmd = ["git", "status", "--porcelain"]
    res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if res.returncode != 0:
        return []

    modified_files: List[Path] = []
    for line in res.stdout.splitlines():
        if not line.strip():
            continue
        status = line[:2]
        path_str = line[3:].strip()

        # Skip deleted files
        if "D" in status:
            continue

        # Handle renamed files 'R  old -> new'
        if " -> " in path_str:
            path_str = path_str.split(" -> ")[1].strip()

        p = Path(path_str).resolve()
        if p.exists() and p.is_file():
            modified_files.append(p)

    return modified_files


def _dispatch_formatter(
    func, config_path: Path, style_path: Optional[Path], files: Optional[List[Path]]
) -> bool:
    """Invokes formatter function passing file filter if supported by signature."""
    sig = inspect.signature(func)
    if "files" in sig.parameters:
        return func(config_path, style_path, files=files)
    elif len(sig.parameters) >= 3:
        return func(config_path, style_path, files)
    return func(config_path, style_path)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Multi-language code formatter based on resolved YAML configuration."
    )

    parser.add_argument(
        "-c",
        "--config",
        type=Path,
        required=True,
        help="Path to the resolved YAML configuration file.",
    )
    parser.add_argument(
        "-l",
        "--lang",
        choices=["all", "c", "python", "yaml"],
        default="all",
        help="Target language: 'c', 'python', 'yaml', or 'all' (default: all)",
    )
    parser.add_argument(
        "-m",
        "--mode",
        choices=["apply", "check"],
        default="apply",
        help="Action mode: 'apply' or 'check' (default: apply)",
    )
    parser.add_argument(
        "-s",
        "--scope",
        choices=["all", "changed"],
        default="changed",
        help="Scope: 'changed' (only uncommitted Git changes) or 'all' (entire repo) (default: changed)",
    )
    parser.add_argument(
        "-csc",
        "--c-style-config",
        type=Path,
        default=None,
        help="Optional override path for C/C++ style config (.clang-format)",
    )
    parser.add_argument(
        "-psc",
        "--python-style-config",
        type=Path,
        default=None,
        help="Optional override path for Python style config (pyproject.toml)",
    )

    args = parser.parse_args()
    success = True

    # Resolve target files if scope is git-constrained
    target_files: Optional[List[Path]] = None
    if args.scope == "changed":
        target_files = get_git_modified_files()
        print(f"[*] Git scope: {len(target_files)} modified file(s) detected.")
        if not target_files:
            print("✅ No modified files to format.")
            sys.exit(0)

    # 1. C / C++ Execution
    if args.lang in ("c", "all"):
        if args.mode == "check":
            passed = _dispatch_formatter(
                format_c_check, args.config, args.c_style_config, target_files
            )
        else:
            passed = _dispatch_formatter(
                format_c_apply, args.config, args.c_style_config, target_files
            )
        success = success and passed

    # 2. Python Execution (using your in-repo script)
    if args.lang in ("python", "all"):
        if args.mode == "check":
            passed = _dispatch_formatter(
                format_python_check, args.config, args.python_style_config, target_files
            )
        else:
            passed = _dispatch_formatter(
                format_python_apply, args.config, args.python_style_config, target_files
            )
        success = success and passed

    # 3. YAML Execution
    if args.lang in ("yaml", "all"):
        if args.mode == "check":
            passed = format_yaml_check(args.config, files=target_files)
        else:
            passed = format_yaml_apply(args.config, files=target_files)
        success = success and passed

    if not success and args.mode == "check":
        sys.exit(1)


if __name__ == "__main__":
    main()
