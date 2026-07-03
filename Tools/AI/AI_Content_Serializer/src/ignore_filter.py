# src/ignore_filter.py
import re
from pathlib import Path
import config


class GitignoreMatcher:
    def __init__(self, patterns: list[str]):
        self.rules = []
        for pat in patterns:
            stripped = pat.strip()
            # Ignore empty lines and comments
            if not stripped or stripped.startswith("#"):
                continue

            is_negated = stripped.startswith("!")
            if is_negated:
                stripped = stripped[1:]

            is_dir_only = stripped.endswith("/")
            if is_dir_only:
                stripped = stripped[:-1]

            # Root anchoring: if there is a slash anywhere (excluding trailing),
            # it is relative to the root directory
            is_anchored = stripped.startswith("/")
            if is_anchored:
                stripped = stripped[1:]
            elif "/" in stripped:
                is_anchored = True

            regex = self._translate_to_regex(stripped, is_anchored)
            self.rules.append(
                {
                    "regex": regex,
                    "is_negated": is_negated,
                    "is_dir_only": is_dir_only,
                    "raw": pat,
                }
            )

    def _translate_to_regex(self, pattern: str, is_anchored: bool) -> re.Pattern:
        """Translates gitignore glob patterns into safe, compiled Python regexes."""
        escaped = re.escape(pattern)

        # Convert wildcards using placeholders to prevent double-escaping conflicts
        temp = escaped.replace("\\*\\*", "___ANY_DIR___")
        temp = temp.replace("\\*", "___STAR___")
        temp = temp.replace("\\?", "___QUESTION___")

        # Swap placeholders with correct regex matching
        temp = temp.replace("___ANY_DIR___", ".*")
        temp = temp.replace("___STAR___", "[^/]*")
        temp = temp.replace("___QUESTION___", "[^/]")

        # Anchoring rules
        if is_anchored:
            regex_str = f"^{temp}(?:/.*)?$"
        else:
            regex_str = f"^(?:.*/)?{temp}(?:/.*)?$"

        return re.compile(regex_str)

    def is_ignored(self, relative_path: Path, is_dir: bool) -> bool:
        """Evaluates path against compiled rules sequentially. Last matching pattern wins."""
        ignored = False
        path_str = relative_path.as_posix()  # Normalizes backslashes to forward slashes

        for rule in self.rules:
            if rule["is_dir_only"] and not is_dir:
                continue

            if rule["regex"].match(path_str):
                ignored = not rule["is_negated"]

        return ignored


# Caching matcher instances per root directory to optimize performance
_matchers_cache = {}


def get_matcher_for_dir(base_dir: Path) -> GitignoreMatcher:
    """Retrieves or builds the GitignoreMatcher, combining config defaults with any local .gitignore file."""
    if base_dir not in _matchers_cache:
        patterns = list(config.IGNORE_PATTERNS)

        # Automatically check for and load an actual .gitignore file on disk
        local_gitignore = base_dir / ".gitignore"
        if local_gitignore.exists() and local_gitignore.is_file():
            try:
                lines = (
                    local_gitignore.read_text(encoding="utf-8").splitlines()
                )
                patterns.extend(lines)
            except Exception as e:
                print(f"[Warning] Failed to read local .gitignore: {e}")

        _matchers_cache[base_dir] = GitignoreMatcher(patterns)
    return _matchers_cache[base_dir]


def should_ignore(path: Path, base_dir: Path) -> bool:
    """Evaluates if a path is ignored using custom .gitignore rules."""
    try:
        relative_path = path.relative_to(base_dir)
    except ValueError:
        return False

    matcher = get_matcher_for_dir(base_dir)
    return matcher.is_ignored(relative_path, path.is_dir())