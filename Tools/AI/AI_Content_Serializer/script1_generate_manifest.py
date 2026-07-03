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
    r"""Usage Example:
    python script1_generate_manifest.py /path/to/directory
    """
    default_manifest = Path(config.OUTPUT_DIR) / config.DEFAULT_MANIFEST_NAME
    default_tree = Path(config.OUTPUT_DIR) / config.DEFAULT_TREE_NAME

    parser = argparse.ArgumentParser(
        description="Scan a folder to output a directory tree and a configurable JSON manifest."
    )
    parser.add_argument(
        "parent_dir",
        type=str,
        help="Path to the parent directory to scan.",
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

    parent_dir = Path(args.parent_dir).resolve()
    manifest_path = Path(args.output_manifest).resolve()
    tree_path = Path(args.output_tree).resolve()

    if not parent_dir.exists() or not parent_dir.is_dir():
        print(f"Error: Directory '{parent_dir}' does not exist.")
        return

    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    tree_path.parent.mkdir(parents=True, exist_ok=True)

    print(f"Scanning target directory: {parent_dir}")
    tree_lines, files_list = build_tree_and_files(parent_dir, parent_dir)

    tree_content = [parent_dir.name] + tree_lines
    tree_text = "\n".join(tree_content)

    write_text_file(tree_path, tree_text)
    print(f"Saved directory tree preview to: {tree_path}")

    # Build the default file list
    initial_files = []
    for file_path in files_list:
        relative_path = file_path.relative_to(parent_dir)
        initial_files.append(
            {"relative_path": str(relative_path), "include": True}
        )

    # --- ADDED INTERACTIVE GUI STEP ---
    print("Launching interactive file selector window...")
    configured_files = select_files_interactively(initial_files)

    manifest_data = {
        "parent_directory": str(parent_dir),
        "tree_structure": tree_text,
        "files": configured_files,
    }

    save_manifest(manifest_path, manifest_data)
    print(f"Saved selected configuration to: {manifest_path}")


if __name__ == "__main__":
    main()