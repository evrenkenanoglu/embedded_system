# script2_serialize.py
import os
import argparse
from pathlib import Path
import config
from src.manifest_manager import load_manifest, write_text_file
from src.serializer import generate_markdown


def parse_arguments() -> argparse.Namespace:
    """Parses command-line arguments for the serialization step."""
    # Resolve default paths relative to the OUT directory specified in config.py
    default_manifest = Path(config.OUTPUT_DIR) / config.DEFAULT_MANIFEST_NAME
    default_output = Path(config.OUTPUT_DIR) / config.DEFAULT_SERIALIZATION_NAME

    parser = argparse.ArgumentParser(
        description="Serialize selected files from the JSON manifest into a single structured output."
    )
    parser.add_argument(
        "-m",
        "--manifest",
        type=str,
        default=str(default_manifest),
        help=f"Path to the JSON manifest file (default: {default_manifest}).",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        default=str(default_output),
        help=f"Path to save the output text/markdown file (default: {default_output}).",
    )
    return parser.parse_args()


def get_common_ancestor(paths_list: list) -> Path:
    """Finds the lowest common ancestor directory for a list of Path objects."""
    if not paths_list:
        return Path("")
    common_path = os.path.commonpath([str(p) for p in paths_list])
    return Path(common_path).resolve()


def main():
    args = parse_arguments()

    manifest_path = Path(args.manifest).resolve()
    output_path = Path(args.output).resolve()

    if not manifest_path.exists():
        print(f"Error: Manifest file '{manifest_path}' does not exist.")
        return

    # Ensure the directory for the serialized output exists
    output_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        manifest_data = load_manifest(manifest_path)
    except Exception as e:
        print(f"Error reading manifest: {e}")
        return

    # Parse new multi-directory key with backward compatibility fallback
    input_dirs_str = manifest_data.get("input_directories", [])
    if not input_dirs_str:
        legacy_dir = manifest_data.get("parent_directory") or manifest_data.get(
            "input_dirsectory"
        )
        if legacy_dir:
            input_dirs_str = [legacy_dir]
        else:
            print("Error: Missing input directories configuration in manifest.")
            return

    resolved_input_dirs = [Path(d).resolve() for d in input_dirs_str]
    common_parent = get_common_ancestor(resolved_input_dirs)

    files_list = manifest_data.get("files", [])
    tree_structure = manifest_data.get("tree_structure", "")

    # Reconstruct paths relative to the common parent directory
    normalized_files_list = []
    for f in files_list:
        if f.get("include"):
            base_dir = Path(f.get("base_dir", common_parent))
            relative_path = Path(f.get("relative_path", ""))

            # Construct absolute path, then resolve relative to the common parent
            abs_path = (base_dir / relative_path).resolve()
            try:
                rel_to_parent = abs_path.relative_to(common_parent)
            except ValueError:
                # Fallback to absolute path if on different drives (Windows)
                rel_to_parent = abs_path

            modified_entry = f.copy()
            modified_entry["relative_path"] = str(rel_to_parent)
            normalized_files_list.append(modified_entry)
        else:
            normalized_files_list.append(f)

    markdown_output, processed_files = generate_markdown(
        common_parent, tree_structure, normalized_files_list
    )

    write_text_file(output_path, markdown_output)

    print(
        f"Successfully processed {processed_files} files relative to: {common_parent}"
    )
    print(f"Serialized context saved to: {output_path}")


if __name__ == "__main__":
    main()
