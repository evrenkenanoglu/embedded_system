# ignore_filter.py
from pathlib import Path
import config


def should_ignore(path: Path, base_dir: Path) -> bool:
    """Evaluates if a given path or file matches the ignore configurations."""
    try:
        relative = path.relative_to(base_dir)
        for part in relative.parts:
            if part in config.DEFAULT_IGNORE_DIRS:
                return True
    except ValueError:
        pass

    if (
        path.is_file()
        and path.suffix.lower() in config.DEFAULT_IGNORE_EXTENSIONS
    ):
        return True

    return False