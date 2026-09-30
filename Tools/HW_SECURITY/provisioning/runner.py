#!/usr/bin/env python3
"""
@file       runner.py
@brief      Interactive and shorthand execution wrapper for main.py.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved

USAGE EXAMPLES:
    1. Zero-argument Interactive Menu (Auto-detects configs/config_provisioning.yaml):
        python embedded_system/Tools/HW_SECURITY/provisioning/runner.py

    2. Explicit Config File:
        python embedded_system/Tools/HW_SECURITY/provisioning/runner.py --config configs/config_provisioning.yaml --all --dry-run

    3. Shorthand Steps:
        python embedded_system/Tools/HW_SECURITY/provisioning/runner.py --nvs
        python embedded_system/Tools/HW_SECURITY/provisioning/runner.py --sign
        python embedded_system/Tools/HW_SECURITY/provisioning/runner.py --provision --dry-run
"""

import argparse
import subprocess
import sys
from pathlib import Path
from typing import Optional

SCRIPT_DIR = Path(__file__).resolve().parent
MAIN_PY = SCRIPT_DIR / "main.py"


def resolve_default_config() -> Path:
    """Finds config_provisioning.yaml checking configs/ directory before SCRIPT_DIR."""
    candidates = [
        Path.cwd() / "configs" / "config_provisioning.yaml",
        Path.cwd() / "config_provisioning.yaml",
        SCRIPT_DIR / "config.yaml",
        SCRIPT_DIR / "config_provisioning.yaml",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate.resolve()
    return (Path.cwd() / "configs" / "config_provisioning.yaml").resolve()


def run_main(
    config_path: Path,
    step: str,
    target_mode: str = "virtual",
    burn_password: Optional[str] = None,
    continuous: bool = False,
    project_root: Optional[Path] = None,
    dry_run: bool = False,
) -> int:
    """Executes main.py with resolved configuration and step arguments."""
    cmd = [
        sys.executable,
        str(MAIN_PY),
        "--config",
        str(config_path),
        "--step",
        step,
        "--target-mode",
        target_mode,
    ]

    if project_root:
        cmd.extend(["--project-root", str(project_root)])
    if burn_password:
        cmd.extend(["--burn-password", burn_password])
    if continuous:
        cmd.append("--continuous")
    if dry_run:
        cmd.append("--dry-run")

    print(f"\n[*] Executing: {' '.join(cmd)}\n")
    result = subprocess.run(cmd)
    return result.returncode


def interactive_menu(config_path: Path, project_root: Optional[Path] = None) -> None:
    """Displays an interactive selection menu when no step flags are provided."""
    while True:
        print("\n" + "=" * 55)
        print(" ESP32-S3 PROVISIONING & RELEASE RUNNER")
        print("=" * 55)
        print(f" Active Config: {config_path}")
        if project_root:
            print(f" Project Root : {project_root}")
        print("-" * 55)
        print(" [1] Full Pipeline (Virtual Mode / Safe Dry-Run)")
        print(" [2] Full Pipeline (Hardware Mode - Single Unit)")
        print(" [3] Full Pipeline (Hardware Mode - Continuous Assembly)")
        print(" [4] Generate Encrypted NVS Partition Only")
        print(" [5] Sign Firmware Binary & Update Manifest")
        print(" [6] Hardware Silicon Provisioning Only (Virtual)")
        print(" [7] Hardware Silicon Provisioning Only (Physical Flash)")
        print(" [0] Exit")
        print("=" * 55)

        choice = input("\nSelect option [0-7]: ").strip()

        if choice == "1":
            run_main(config_path, "all", target_mode="virtual", dry_run=True, project_root=project_root)
            break
        elif choice == "2":
            pwd = input("Enter burn password for hardware flash: ").strip()
            run_main(config_path, "all", target_mode="hardware", burn_password=pwd, project_root=project_root)
            break
        elif choice == "3":
            pwd = input("Enter burn password for hardware flash: ").strip()
            run_main(config_path, "all", target_mode="hardware", burn_password=pwd, continuous=True, project_root=project_root)
            break
        elif choice == "4":
            run_main(config_path, "nvs", project_root=project_root)
            break
        elif choice == "5":
            run_main(config_path, "sign", project_root=project_root)
            break
        elif choice == "6":
            run_main(config_path, "provision", target_mode="virtual", dry_run=True, project_root=project_root)
            break
        elif choice == "7":
            pwd = input("Enter burn password for hardware flash: ").strip()
            run_main(config_path, "provision", target_mode="hardware", burn_password=pwd, project_root=project_root)
            break
        elif choice == "0":
            print("\nExiting.")
            sys.exit(0)
        else:
            print("\n[ERROR] Invalid option. Please enter a number from 0 to 7.")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Quick runner for ESP32 provisioning operations.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--config",
        "-c",
        type=Path,
        default=None,
        help="Path to config_provisioning.yaml",
    )
    parser.add_argument(
        "--project-root",
        "-r",
        type=Path,
        default=None,
        help="Explicit project root directory override",
    )
    parser.add_argument(
        "--step",
        "-s",
        choices=["all", "nvs", "sign", "provision"],
        default=None,
        help="Target step to execute",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Run full pipeline (NVS -> Sign -> Provision)",
    )
    parser.add_argument(
        "--nvs", action="store_true", help="Generate encrypted NVS partition only"
    )
    parser.add_argument(
        "--sign", action="store_true", help="Sign firmware and update server manifest"
    )
    parser.add_argument(
        "--provision", action="store_true", help="Provision hardware silicon"
    )
    parser.add_argument(
        "--target-mode",
        choices=["virtual", "hardware"],
        default="virtual",
        help="Execution target: 'virtual' or 'hardware'",
    )
    parser.add_argument(
        "--burn-password",
        type=str,
        default=None,
        help="Passphrase for hardware eFuse burns",
    )
    parser.add_argument(
        "--continuous",
        action="store_true",
        help="Continuous assembly-line batch mode",
    )
    parser.add_argument(
        "--dry-run",
        "-d",
        action="store_true",
        help="Run in dry-run mode (no eFuse burning)",
    )
    args = parser.parse_args()

    config_file = args.config.resolve() if args.config else resolve_default_config()

    # Determine step from --step argument or shorthand flags
    selected_step = args.step
    if not selected_step:
        if args.all:
            selected_step = "all"
        elif args.nvs:
            selected_step = "nvs"
        elif args.sign:
            selected_step = "sign"
        elif args.provision:
            selected_step = "provision"

    # Launch interactive menu if no step arguments were passed
    if not selected_step:
        interactive_menu(config_file, project_root=args.project_root)
        return 0

    return run_main(
        config_file,
        selected_step,
        target_mode=args.target_mode,
        burn_password=args.burn_password,
        continuous=args.continuous,
        project_root=args.project_root,
        dry_run=args.dry_run,
    )


if __name__ == "__main__":
    sys.exit(main())
