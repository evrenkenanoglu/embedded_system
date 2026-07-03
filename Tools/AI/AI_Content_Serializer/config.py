# config.py

# Output configuration
OUTPUT_DIR = "OUT"
DEFAULT_MANIFEST_NAME = "project_manifest.json"
DEFAULT_TREE_NAME = "directory_tree.txt"
DEFAULT_SERIALIZATION_NAME = "ai_context.md"

# Gitignore-style patterns to ignore
# Supports directories (trailing /), wildcards (*), negation (!), and root anchoring (leading /)
IGNORE_PATTERNS = [
    # Directories
    ".git/",
    "__pycache__/",
    "node_modules/",
    ".venv/",
    "venv/",
    "env/",
    ".idea/",
    ".vscode/",
    "build/",
    "dist/",
    "OUT/",  # Automatically ignores the output folder

    # Extensions / Wildcards
    "*.pyc",
    "*.pyo",
    "*.pyd",
    "*.png",
    "*.jpg",
    "*.jpeg",
    "*.gif",
    "*.svg",
    "*.ico",
    "*.zip",
    "*.tar",
    "*.gz",
    "*.pdf",
    "*.db",
    "*.sqlite",
    "__init__.py",  # Ignore Python package init files

    # Specific file names
    ".DS_Store",
    "Thumbs.db",
]


# GUI Font configurations (No hardcoded values)
GUI_FONT_FAMILY = "Arial"
GUI_FONT_SIZE_BASE = 12  # Font size for buttons and general interface labels
GUI_FONT_SIZE_TREE = 12  # Font size for the main interactive file tree paths