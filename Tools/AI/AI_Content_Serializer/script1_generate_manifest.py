#!/usr/bin/env python3
# script1_generate_manifest.py

import argparse
from pathlib import Path
import config
from src.scanner import build_tree_and_files
from src.manifest_manager import save_manifest, write_text_file

# --- ADDED IMPORT ---
from src.selector_gui import select_files_interactively


def parse_arguments() -> argparse.Namespace:
    """Parses command-line arguments for generating the manifest."""
    default_manifest = Path(config.OUTPUT_DIR) / config.DEFAULT_MANIFEST_NAME
    default_tree = Path(config.OUTPUT_DIR) / config.DEFAULT_TREE_NAME

    parser = argparse.ArgumentParser(
        description="Scan folders to output a directory tree and a configurable JSON manifest."
    )
    parser.add_argument(
        "--input_dirs",
        "-id",
        type=str,
        nargs="+",  # Supports one or more directories
        required=True,
        help="Path to the parent directories to scan.",
    )
    parser.add_argument(
        "-o",
        "--output-manifest",
        type=str,
        default=str(default_manifest),
        help=f"Path to save the JSON manifest (default: {default_manifest}).",
    )
    parser.add_argument(
        "-t",
        "--output-tree",
        type=str,
        default=str(default_tree),
        help=f"Path to save the visual directory tree (default: {default_tree}).",
    )

    return parser.parse_args()


def main():
    args = parse_arguments()

    manifest_path = Path(args.output_manifest).resolve()
    tree_path = Path(args.output_tree).resolve()

    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    tree_path.parent.mkdir(parents=True, exist_ok=True)

    # Resolve and validate all input directories
    resolved_dirs = []
    for d in args.input_dirs:
        input_dir = Path(d).resolve()
        if not input_dir.exists() or not input_dir.is_dir():
            print(f"Error: Directory '{input_dir}' does not exist.")
            return
        resolved_dirs.append(input_dir)

    combined_tree_lines = []
    initial_files = []

    # Process each directory individually
    for input_dir in resolved_dirs:
        print(f"Scanning target directory: {input_dir}")
        tree_lines, files_list = build_tree_and_files(input_dir, input_dir)

        # Append visual tree structure for this base directory
        combined_tree_lines.append(input_dir.name)
        combined_tree_lines.extend(tree_lines)
        combined_tree_lines.append("")  # Blank separator line

        # Build file list tracking relative path and associated base_dir
        for file_path in files_list:
            relative_path = file_path.relative_to(input_dir)
            initial_files.append(
                {
                    "base_dir": str(input_dir),
                    "relative_path": str(relative_path),
                    "include": True,
                }
            )

    # Write combined directory tree
    tree_text = "\n".join(combined_tree_lines).strip()
    write_text_file(tree_path, tree_text)
    print(f"Saved combined directory tree preview to: {tree_path}")

    # Launch GUI selector step
    print("Launching interactive file selector window...")
    configured_files = select_files_interactively(initial_files)

    manifest_data = {
        "input_directories": [str(d) for d in resolved_dirs],
        "tree_structure": tree_text,
        "files": configured_files,
    }

    save_manifest(manifest_path, manifest_data)
    print(f"Saved selected configuration to: {manifest_path}")


if __name__ == "__main__":
    main()