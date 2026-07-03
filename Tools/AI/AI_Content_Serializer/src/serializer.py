# serializer.py
from pathlib import Path


def get_markdown_language(path: Path) -> str:
    """Maps common extensions to markdown-supported programming languages."""
    suffix = path.suffix.lower()
    mapping = {
        ".py": "python",
        ".js": "javascript",
        ".ts": "typescript",
        ".json": "json",
        ".md": "markdown",
        ".html": "html",
        ".css": "css",
        ".sh": "bash",
        ".yml": "yaml",
        ".yaml": "yaml",
        ".sql": "sql",
        ".txt": "text",
    }
    return mapping.get(suffix, "")


def generate_markdown(
    parent_dir: Path, tree_structure: str, files_list: list[dict]
) -> tuple[str, int]:
    """Generates a combined markdown block string of all configured files."""
    serialized_content = []

    # Format Directory Structure Section
    serialized_content.append("# Project Structure\n")
    serialized_content.append("```text")
    serialized_content.append(tree_structure)
    serialized_content.append("```\n")

    # Format File Contents Section
    serialized_content.append("# File Contents\n")

    included_count = 0
    for file_entry in files_list:
        relative_path_str = file_entry.get("relative_path")
        include = file_entry.get("include", True)

        if not include:
            continue

        file_path = (parent_dir / relative_path_str).resolve()

        if not file_path.exists() or not file_path.is_file():
            continue

        serialized_content.append(f"## File: `{relative_path_str}`")

        lang = get_markdown_language(file_path)
        serialized_content.append(f"```{lang}")

        try:
            file_text = file_path.read_text(encoding="utf-8", errors="replace")
            serialized_content.append(file_text)
        except Exception as e:
            serialized_content.append(f"[Error reading file content: {e}]")

        serialized_content.append("```\n")
        included_count += 1

    return "\n".join(serialized_content), included_count