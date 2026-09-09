import re
from pathlib import Path
from typing import Any, Dict, Union
import yaml


class ConfigNode(dict):
    """Allows attribute dot notation access on configuration dictionaries."""

    def __getattr__(self, name: str) -> Any:
        try:
            val = self[name]
            if isinstance(val, dict) and not isinstance(val, ConfigNode):
                return ConfigNode(val)
            return val
        except KeyError:
            raise AttributeError(f"Configuration key '{name}' not found.")

    def __setattr__(self, name: str, value: Any) -> None:
        self[name] = value


def _flatten_dict(d: Dict[str, Any], parent_key: str = "", sep: str = ".") -> Dict[str, str]:
    items = []
    for k, v in d.items():
        new_key = f"{parent_key}{sep}{k}" if parent_key else k
        if isinstance(v, dict):
            items.extend(_flatten_dict(v, new_key, sep=sep).items())
        elif isinstance(v, (str, int, float, bool)):
            items.append((new_key, str(v)))
    return dict(items)


def _expand_placeholders(data: Any, context: Dict[str, str]) -> Any:
    if isinstance(data, str):
        pattern = re.compile(r"\{([\w\.]+)\}")
        for _ in range(5):
            matches = pattern.findall(data)
            if not matches:
                break
            for match in matches:
                if match in context:
                    data = data.replace(f"{{{match}}}", context[match])
        return data
    elif isinstance(data, dict):
        return {k: _expand_placeholders(v, context) for k, v in data.items()}
    elif isinstance(data, list):
        return [_expand_placeholders(item, context) for item in data]
    return data


def load_config(config_path: Union[str, Path], workspace_root: Union[str, Path, None] = None) -> ConfigNode:
    """Load a YAML configuration file, bind project_root, and resolve placeholders."""
    cfg_file = Path(config_path).resolve()
    if not cfg_file.exists():
        raise FileNotFoundError(f"Configuration file not found at: {cfg_file}")

    with open(cfg_file, "r", encoding="utf-8") as f:
        raw_cfg = yaml.safe_load(f) or {}

    root = Path(workspace_root).resolve() if workspace_root else cfg_file.parent.parent
    raw_cfg["project_root"] = str(root)

    # Dynamic release naming evaluation
    if raw_cfg.get("build", {}).get("release", False):
        version = raw_cfg.get("build", {}).get("version", "0.1.0")
        name = raw_cfg.get("project_name", "app")
        raw_cfg.setdefault("package", {})["name"] = f"package_{name}_v{version}"
        raw_cfg["package"]["artifact_name"] = f"{{output_dir}}/{name}_v{version}"

    context = _flatten_dict(raw_cfg)
    expanded = _expand_placeholders(raw_cfg, context)

    return ConfigNode(expanded)