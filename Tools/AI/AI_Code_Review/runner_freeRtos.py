#!/usr/bin/env python3
# runner_general.py

import sys
import argparse
import subprocess
from pathlib import Path

# ==============================================================================
# CONFIGURATION DEFAULTS (Hardcode your preferences here)
# ==============================================================================
DEFAULT_PROVIDER = "gemini"

DEFAULT_TEMPLATES = [
    "prompts/freeRtos_review.md",
]

DEFAULT_FILES = [
    # Hardcode default files or directories to review here, e.g.:
    "C:/WORKSPACE_PERSONAL/PROJECTS/SMART_PLUGS/SW/embedded_system/Source/HAL/IHAL/IHal_Mem_Ota.h",
    "C:/WORKSPACE_PERSONAL/PROJECTS/SMART_PLUGS/SW/embedded_system/Source/HAL/Platform/ESP32/mem_ota.cpp",
    "C:/WORKSPACE_PERSONAL/PROJECTS/SMART_PLUGS/SW/embedded_system/Source/HAL/Platform/ESP32/mem_ota.hpp"
]
# ==============================================================================


def main():
    parser = argparse.ArgumentParser(
        description="General Code Review Runner (Static Files & Folders)"
    )
    parser.add_argument(
        "--files",
        nargs="+",
        default=DEFAULT_FILES,
        help="Files or folders to review",
    )
    parser.add_argument(
        "--provider",
        default=DEFAULT_PROVIDER,
        help=f"AI provider (default: {DEFAULT_PROVIDER})",
    )
    parser.add_argument(
        "--templates",
        nargs="+",
        default=DEFAULT_TEMPLATES,
        help="List of prompt template names, markdown files, or folders containing templates",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable verbose debug logging of files and prompt contents",
    )

    args, remaining_args = parser.parse_known_args()

    # Enforce files requirement
    if not args.files:
        print("❌ Error: No files specified for review.")
        print("Please provide files via CLI: --files <path1> <path2>")
        print("Or hardcode them in runner_general.py under 'DEFAULT_FILES'.")
        sys.exit(1)

    # Explicitly forward provider and custom templates to main.py
    remaining_args.extend(["--provider", args.provider])
    remaining_args.extend(["--files"] + args.files)
    if args.templates:
        remaining_args.extend(["--templates"] + args.templates)

    # Explicitly forward the --debug flag to main.py
    if args.debug:
        remaining_args.append("--debug")

    # Debug Logging
    if args.debug:
        print("\n" + "[DEBUG] " + "=" * 45)
        print("[DEBUG] GENERAL STATIC FILE REVIEW METADATA")
        print("[DEBUG] " + "=" * 45)
        print(f"[DEBUG] Target Provider: {args.provider}")
        print(f"[DEBUG] Files to Review: {args.files}")
        print(f"[DEBUG] Templates Used:  {args.templates}")
        print("[DEBUG] " + "=" * 45 + "\n")

    # Forward execution to main.py
    main_script = Path(__file__).parent.absolute() / "main.py"
    cmd = [
        sys.executable,
        str(main_script),
    ] + remaining_args

    print(f"\n🚀 Forwarding to main.py:\n  {' '.join(cmd)}\n")
    result = subprocess.run(cmd)
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()