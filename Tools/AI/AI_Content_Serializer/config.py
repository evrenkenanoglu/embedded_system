# config.py

# Directories to skip entirely
DEFAULT_IGNORE_DIRS = {
    ".git",
    "__pycache__",
    "node_modules",
    ".venv",
    "venv",
    "env",
    ".idea",
    ".vscode",
    "build",
    "dist",
    ""
}

# File extensions to ignore by default (e.g., binaries, media)
DEFAULT_IGNORE_EXTENSIONS = {
    ".pyc",
    ".pyo",
    ".pyd",
    ".png",
    ".jpg",
    ".jpeg",
    ".gif",
    ".svg",
    ".ico",
    ".zip",
    ".tar",
    ".gz",
    ".pdf",
    ".db",
    ".sqlite",
    ".bin",
    ".hex",
    ".elf",
    ".log"
}

# Default naming for output files
OUTPUT_DIR = "OUT"
DEFAULT_MANIFEST_NAME = "project_manifest.json"
DEFAULT_TREE_NAME = "directory_tree.txt"
DEFAULT_SERIALIZATION_NAME = "ai_context.md"
