#!/usr/bin/env python3
# run_pipeline.py

import sys
import argparse
import subprocess
from pathlib import Path

# ==============================================================================
# CONFIGURATION DEFAULTS (Hardcode your preferences here)
# ==============================================================================
DEFAULT_INPUT_DIRS = [
    "C:/WORKSPACE_PERSONAL/PROJECTS/SMART_PLUGS/SW/embedded_system/Documentation/Code_Convention",
    "C:/WORKSPACE_PERSONAL/PROJECTS/SMART_PLUGS/SW/embedded_system/Source/HAL/IHAL",
]

DEFAULT_MANIFEST_PATH = "Out/manifest.json"
DEFAULT_TREE_PATH = "Out/directory_tree.txt"
DEFAULT_SERIALIZE_PATH = "Out/serialized_context.md"
# ==============================================================================


def parse_arguments():
    """Parses command-line arguments for the orchestrator."""
    parser = argparse.ArgumentParser(
        description="Orchestrator to generate manifest and serialize code context."
    )
    parser.add_argument(
        "--input_dirs",
        "-id",
        type=str,
        nargs="+",
        default=DEFAULT_INPUT_DIRS,
        help="Paths to the parent directories to scan.",
    )
    parser.add_argument(
        "-m",
        "--manifest",
        type=str,
        default=DEFAULT_MANIFEST_PATH,
        help=f"Path to the JSON manifest (default: {DEFAULT_MANIFEST_PATH}).",
    )
    parser.add_argument(
        "-t",
        "--tree",
        type=str,
        default=DEFAULT_TREE_PATH,
        help=f"Path to save directory tree preview (default: {DEFAULT_TREE_PATH}).",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        default=DEFAULT_SERIALIZE_PATH,
        help=f"Path to save serialized Markdown output (default: {DEFAULT_SERIALIZE_PATH}).",
    )
    return parser.parse_args()


def main():
    args = parse_arguments()

    script_dir = Path(__file__).parent.absolute()
    script1_path = script_dir / "script1_generate_manifest.py"
    script2_path = script_dir / "script2_serialize.py"

    # 1. Execute Script 1: Manifest Generation (Native list extension)
    print("🚀 Step 1: Generating Manifest...")
    cmd_manifest = (
        [
            sys.executable,
            str(script1_path),
            "--input_dirs",
        ]
        + args.input_dirs
        + ["--output-manifest", args.manifest, "--output-tree", args.tree]
    )

    print(f"Executing: {' '.join(cmd_manifest)}")
    res1 = subprocess.run(cmd_manifest)
    if res1.returncode != 0:
        print("❌ Step 1 failed. Aborting pipeline.")
        sys.exit(res1.returncode)

    # 2. Execute Script 2: Serialization
    print("\n🚀 Step 2: Serializing Context...")
    cmd_serialize = [
        sys.executable,
        str(script2_path),
        "--manifest",
        args.manifest,
        "--output",
        args.output,
    ]

    print(f"Executing: {' '.join(cmd_serialize)}")
    res2 = subprocess.run(cmd_serialize)
    if res2.returncode != 0:
        print("❌ Step 2 failed. Aborting pipeline.")
        sys.exit(res2.returncode)

    print("\n🎉 Pipeline execution completed successfully!")


if __name__ == "__main__":
    main()
