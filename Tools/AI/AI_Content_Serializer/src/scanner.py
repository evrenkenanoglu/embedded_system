# scanner.py
from pathlib import Path
from src.ignore_filter import should_ignore


def build_tree_and_files(
    dir_path: Path, current_dir: Path, prefix: str = ""
) -> tuple[list[str], list[Path]]:
    """Recursively walks a directory to construct structure strings and gather target files."""
    tree_lines = []
    files_list = []

    try:
        items = sorted(
            [
                item
                for item in current_dir.iterdir()
                if not should_ignore(item, dir_path)
            ],
            key=lambda x: (not x.is_dir(), x.name.lower()),
        )
    except PermissionError:
        return tree_lines, files_list

    count = len(items)
    for index, item in enumerate(items):
        is_last = index == count - 1
        connector = "└── " if is_last else "├── "
        tree_lines.append(f"{prefix}{connector}{item.name}")

        if item.is_dir():
            next_prefix = prefix + ("    " if is_last else "│   ")
            sub_lines, sub_files = build_tree_and_files(
                dir_path, item, next_prefix
            )
            tree_lines.extend(sub_lines)
            files_list.extend(sub_files)
        else:
            files_list.append(item)

    return tree_lines, files_list