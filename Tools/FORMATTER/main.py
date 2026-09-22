import argparse
import sys
from pathlib import Path

# Add project directory to Python module search path
sys.path.insert(0, str(Path(__file__).resolve().parent))

from src.format_c_files import format_c_apply, format_c_check
from src.format_python_files import format_python_apply, format_python_check


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Multi-language code formatter (apply/check) based on resolved YAML configuration."
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
        choices=["all", "c", "python"],
        default="all",
        help="Target language: 'c', 'python', or 'all' (default: all)",
    )
    parser.add_argument(
        "-m",
        "--mode",
        choices=["apply", "check"],
        default="apply",
        help="Action mode: 'apply' (format in-place) or 'check' (dry-run check) (default: apply)",
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

    # C / C++ Execution
    if args.lang in ("c", "all"):
        if args.mode == "check":
            passed = format_c_check(args.config, args.c_style_config)
        else:
            passed = format_c_apply(args.config, args.c_style_config)
        success = success and passed

    # Python Execution
    if args.lang in ("python", "all"):
        if args.mode == "check":
            passed = format_python_check(args.config, args.python_style_config)
        else:
            passed = format_python_apply(args.config, args.python_style_config)
        success = success and passed

    if not success and args.mode == "check":
        sys.exit(1)


if __name__ == "__main__":
    main()