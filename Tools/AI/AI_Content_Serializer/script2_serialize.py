# script2_serialize.py
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

    parent_dir_str = manifest_data.get("parent_directory")
    if not parent_dir_str:
        print("Error: Missing parent directory configuration in manifest.")
        return

    parent_dir = Path(parent_dir_str).resolve()
    files_list = manifest_data.get("files", [])
    tree_structure = manifest_data.get("tree_structure", "")

    markdown_output, processed_files = generate_markdown(
        parent_dir, tree_structure, files_list
    )

    write_text_file(output_path, markdown_output)

    print(f"Successfully processed {processed_files} files.")
    print(f"Serialized context saved to: {output_path}")


if __name__ == "__main__":
    main()