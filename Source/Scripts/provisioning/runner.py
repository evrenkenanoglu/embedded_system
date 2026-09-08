#!/usr/bin/env python3
"""
@file       runner.py
@brief      Interactive and shorthand execution wrapper for main.py.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved

USAGE EXAMPLES:
    1. Zero-argument Interactive Menu (Auto-detects configs/config_provisioning.yaml):
        python embedded_system/Source/Scripts/provisioning/runner.py

    2. Explicit Config File:
        python embedded_system/Source/Scripts/provisioning/runner.py --config configs/config_provisioning.yaml --all --dry-run

    3. Shorthand Steps:
        python embedded_system/Source/Scripts/provisioning/runner.py --nvs
        python embedded_system/Source/Scripts/provisioning/runner.py --sign
        python embedded_system/Source/Scripts/provisioning/runner.py --provision --dry-run
"""

import argparse
import subprocess
import sys
from pathlib import Path

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


def run_main(config_path: Path, step: str, dry_run: bool = False) -> int:
    """Executes main.py with resolved configuration and step arguments."""
    cmd = [
        sys.executable,
        str(MAIN_PY),
        "--config", str(config_path),
        "--step", step,
    ]
    if dry_run:
        cmd.append("--dry-run")

    print(f"\n[*] Executing: {' '.join(cmd)}\n")
    result = subprocess.run(cmd)
    return result.returncode


def interactive_menu(config_path: Path) -> None:
    """Displays an interactive selection menu when no step flags are provided."""
    while True:
        print("\n" + "=" * 55)
        print(" ESP32-S3 PROVISIONING & RELEASE RUNNER")
        print("=" * 55)
        print(f" Active Config: {config_path}")
        print("-" * 55)
        print(" [1] Full Pipeline (Dry-Run Validation)")
        print(" [2] Full Pipeline (Physical Silicon Flashing)")
        print(" [3] Generate Encrypted NVS Partition Only")
        print(" [4] Sign Firmware Binary & Update Manifest")
        print(" [5] Hardware Silicon Provisioning (Dry-Run)")
        print(" [6] Hardware Silicon Provisioning (Real Flash)")
        print(" [0] Exit")
        print("=" * 55)

        choice = input("\nSelect option [0-6]: ").strip()

        if choice == "1":
            run_main(config_path, "all", dry_run=True)
            break
        elif choice == "2":
            run_main(config_path, "all", dry_run=False)
            break
        elif choice == "3":
            run_main(config_path, "nvs", dry_run=False)
            break
        elif choice == "4":
            run_main(config_path, "sign", dry_run=False)
            break
        elif choice == "5":
            run_main(config_path, "provision", dry_run=True)
            break
        elif choice == "6":
            run_main(config_path, "provision", dry_run=False)
            break
        elif choice == "0":
            print("\nExiting.")
            sys.exit(0)
        else:
            print("\n[ERROR] Invalid option. Please enter a number from 0 to 6.")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Quick runner for ESP32 provisioning operations.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--config", "-c", type=Path, default=None, help="Path to config_provisioning.yaml")
    parser.add_argument("--all", action="store_true", help="Run full pipeline (NVS -> Sign -> Provision)")
    parser.add_argument("--nvs", action="store_true", help="Generate encrypted NVS partition only")
    parser.add_argument("--sign", action="store_true", help="Sign firmware and update server manifest")
    parser.add_argument("--provision", action="store_true", help="Provision hardware silicon")
    parser.add_argument("--dry-run", "-d", action="store_true", help="Run in dry-run mode (no eFuse burning)")
    args = parser.parse_args()

    config_file = args.config.resolve() if args.config else resolve_default_config()

    # Launch interactive menu if no execution steps were flagged
    if not (args.all or args.nvs or args.sign or args.provision):
        interactive_menu(config_file)
        return 0

    if args.all:
        return run_main(config_file, "all", dry_run=args.dry_run)
    elif args.nvs:
        return run_main(config_file, "nvs", dry_run=args.dry_run)
    elif args.sign:
        return run_main(config_file, "sign", dry_run=args.dry_run)
    elif args.provision:
        return run_main(config_file, "provision", dry_run=args.dry_run)

    return 0


if __name__ == "__main__":
    sys.exit(main())