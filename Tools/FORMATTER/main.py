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
from src.format_cmake_files import format_cmake_apply, format_cmake_check
from src.utils import get_root_dir, load_config


def get_git_modified_files(
    workspace_root: Path, scan_dirs: Optional[List[str]] = None
) -> List[Path]:
    """Collects changed files from the workspace Git root and configured nested repos."""
    repo_roots = [workspace_root]
    for scan_dir in scan_dirs or []:
        candidate = Path(scan_dir)
        if not candidate.is_absolute():
            candidate = workspace_root / candidate
        candidate = candidate.resolve()
        if (
            candidate != workspace_root
            and candidate.is_dir()
            and (candidate / ".git").exists()
        ):
            repo_roots.append(candidate)

    modified_files: List[Path] = []
    seen_files = set()
    for repo_root in repo_roots:
        res = subprocess.run(
            ["git", "status", "--porcelain", "--untracked-files=all"],
            cwd=repo_root,
            capture_output=True,
            text=True,
            check=False,
        )
        if res.returncode != 0:
            continue

        for line in res.stdout.splitlines():
            if not line.strip():
                continue
            status = line[:2]
            path_str = line[3:].strip()

            if "D" in status:
                continue
            if " -> " in path_str:
                path_str = path_str.split(" -> ")[1].strip()

            path = (repo_root / path_str).resolve()
            if (
                path.is_file()
                and path.is_relative_to(workspace_root)
                and path not in seen_files
            ):
                modified_files.append(path)
                seen_files.add(path)

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
        choices=["all", "c", "python", "yaml", "cmake"],
        default="all",
        help="Target language: 'c', 'python', 'yaml', 'cmake', or 'all' (default: all)",
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
        formatter_config = load_config(args.config)
        workspace_root = get_root_dir(formatter_config, args.config)
        scan_dirs = formatter_config.get("workspace", {}).get("scan_dirs", [])
        target_files = get_git_modified_files(workspace_root, scan_dirs)
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

    # 4. CMake Execution
    if args.lang in ("cmake", "all"):
        if args.mode == "check":
            passed = _dispatch_formatter(
                format_cmake_check, args.config, None, target_files
            )
        else:
            passed = _dispatch_formatter(
                format_cmake_apply, args.config, None, target_files
            )
        success = success and passed

    if not success and args.mode == "check":
        sys.exit(1)


if __name__ == "__main__":
    main()
