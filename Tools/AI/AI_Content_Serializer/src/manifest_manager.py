# manifest_manager.py
import json
from pathlib import Path


def save_manifest(manifest_path: Path, manifest_data: dict) -> None:
    """Saves the directory configuration map to a JSON manifest file."""
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest_data, f, indent=4)


def load_manifest(manifest_path: Path) -> dict:
    """Loads and returns the dictionary object from the JSON manifest file."""
    with open(manifest_path, "r", encoding="utf-8") as f:
        return json.load(f)


def write_text_file(output_path: Path, content: str) -> None:
    """Utility helper to write raw strings securely to a text file."""
    output_path.write_text(content, encoding="utf-8")