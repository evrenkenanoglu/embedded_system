import os
import re
import sys
from pathlib import Path
from typing import Any
import pathspec
import yaml

VAR_PATTERN = re.compile(r"\$\{([A-Za-z0-9_]+)(?::-([^}]*))?\}")


def _interpolate_value(value: Any, context: dict[str, str]) -> Any:
    if isinstance(value, str):

        def _replace_match(match: re.Match) -> str:
            var_name = match.group(1)
            default_val = match.group(2)
            if var_name in context:
                return str(context[var_name])
            if var_name in os.environ:
                return os.environ[var_name]
            if default_val is not None:
                return default_val
            return match.group(0)

        return VAR_PATTERN.sub(_replace_match, value)

    elif isinstance(value, dict):
        return {k: _interpolate_value(v, context) for k, v in value.items()}
    elif isinstance(value, list):
        return [_interpolate_value(v, context) for v in value]

    return value


def load_config(config_path: Path) -> dict:
    if not config_path.is_file():
        print(f"Error: Config file '{config_path}' not found.", file=sys.stderr)
        sys.exit(1)

    with open(config_path, "r", encoding="utf-8") as f:
        raw_config = yaml.safe_load(f) or {}

    context: dict[str, str] = {
        "CONFIG_DIR": str(config_path.resolve().parent),
    }

    user_vars = raw_config.get("vars") or raw_config.get("variables") or {}
    for k, v in user_vars.items():
        context[str(k)] = str(_interpolate_value(v, context))

    return _interpolate_value(raw_config, context)


def get_root_dir(config: dict, config_path: Path) -> Path:
    root_val = (
        config.get("root")
        or config.get("workspace", {}).get("root")
        or config.get("paths", {}).get("workspace_dir")
        or "."
    )
    root_path = Path(root_val)
    if root_path.is_absolute():
        return root_path.resolve()
    return (Path.cwd() / root_path).resolve()


def resolve_path(base_dir: Path, path_str: str | Path | None) -> Path | None:
    if not path_str:
        return None
    path = Path(path_str)
    return path if path.is_absolute() else (base_dir / path).resolve()


def _normalize_ignore_patterns(patterns: list[str], root_dir: Path) -> list[str]:
    """Cleans patterns by removing leading './', fixing absolute paths, and standardizing slashes."""
    normalized = []
    for pattern in patterns:
        if not pattern:
            continue
        p = str(pattern).strip()

        # If it was interpolated to an absolute path, make it relative to root_dir
        if Path(p).is_absolute():
            try:
                p = Path(p).relative_to(root_dir).as_posix()
            except ValueError:
                pass

        # Strip leading './' (e.g., './build/**' -> 'build/**')
        while p.startswith("./"):
            p = p[2:]

        if p:
            normalized.append(p)
    return normalized


def find_files(
    root_dir: Path, extensions: tuple[str, ...], ignore_patterns: list[str]
) -> list[Path]:
    """Finds matching files, properly pruning and ignoring directories."""
    if not root_dir.exists():
        print(f"Error: Root directory '{root_dir}' does not exist.", file=sys.stderr)
        sys.exit(1)

    # Clean patterns so gitwildmatch matches relative paths accurately
    cleaned_patterns = _normalize_ignore_patterns(ignore_patterns, root_dir)
    spec = pathspec.PathSpec.from_lines(
        pathspec.patterns.GitWildMatchPattern, cleaned_patterns
    )

    matching_files: list[Path] = []

    # Using os.walk to prune ignored directories early (e.g. avoid scanning entire build/)
    for dirpath, dirnames, filenames in os.walk(root_dir):
        rel_dir = Path(dirpath).relative_to(root_dir).as_posix()
        if rel_dir != ".":
            # Prune directory if ignored
            if spec.match_file(rel_dir + "/"):
                dirnames.clear()
                continue

        for filename in filenames:
            file_path = Path(dirpath) / filename
            if file_path.suffix in extensions:
                rel_file = file_path.relative_to(root_dir).as_posix()
                if not spec.match_file(rel_file):
                    matching_files.append(file_path)

    return matching_files
