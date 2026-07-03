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