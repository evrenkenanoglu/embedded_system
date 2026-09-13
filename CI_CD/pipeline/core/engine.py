import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Union
import yaml

from .docker_manager import DockerManager
from .git_manager import GitManager
from .utils import create_package, create_zip, extract_zip, print_stage

IS_WINDOWS = platform.system() == "Windows"


class PipelineEngine:
    """Generic, schema-driven pipeline orchestrator for host, Docker, and artifact tasks."""

    def __init__(self, config_path: Union[str, Path], workspace_root: Path):
        self.workspace_root = Path(workspace_root).resolve()
        self.config_path = Path(config_path).resolve()
        self.cfg = self._load_and_resolve(self.config_path)
        self.docker_manager = DockerManager(str(self.workspace_root))

    def _flatten_dict(self, d: Dict[str, Any], parent_key: str = "", sep: str = ".") -> Dict[str, str]:
        items = []
        for k, v in d.items():
            new_key = f"{parent_key}{sep}{k}" if parent_key else k
            if isinstance(v, dict):
                items.extend(self._flatten_dict(v, new_key, sep=sep).items())
            elif isinstance(v, (str, int, float, bool)):
                items.append((new_key, str(v)))
        return dict(items)

    def _expand_tokens(self, data: Any, context: Dict[str, str]) -> Any:
        if isinstance(data, str):
            pattern = re.compile(r"\{([\w\.]+)\}")
            matches = pattern.findall(data)
            for match in matches:
                if match in context:
                    data = data.replace(f"{{{match}}}", context[match])
            return data
        elif isinstance(data, dict):
            return {k: self._expand_tokens(v, context) for k, v in data.items()}
        elif isinstance(data, list):
            return [self._expand_tokens(item, context) for item in data]
        return data

    def _load_and_resolve(self, path: Path) -> Dict[str, Any]:
        with open(path, "r", encoding="utf-8") as f:
            stage_cfg = yaml.safe_load(f) or {}

        base_cfg = {}
        base_file = stage_cfg.get("base_config")
        if base_file:
            base_path = (path.parent / base_file).resolve()
            if base_path.exists():
                with open(base_path, "r", encoding="utf-8") as bf:
                    base_cfg = yaml.safe_load(bf) or {}

        # Merge base configuration with stage configuration
        merged = base_cfg.copy()
        for k, v in stage_cfg.items():
            if isinstance(v, dict) and k in merged and isinstance(merged[k], dict):
                merged[k].update(v)
            else:
                merged[k] = v

        # Inject root workspace aliases
        root_str = str(self.workspace_root)
        merged.setdefault("paths", {})["project_root"] = root_str

        # Dynamic OS port resolution for HIL
        hil_cfg = merged.setdefault("hil", {})
        serial_ports = hil_cfg.get("serial_ports", {})
        hil_cfg["resolved_port"] = serial_ports.get("windows" if IS_WINDOWS else "linux", "AUTO")

        # Multi-pass iterative resolution to resolve chained dependencies
        for _ in range(10):
            context = self._flatten_dict(merged)
            # Add root aliases so {project_root} and {paths.project_root} both match
            context["project_root"] = root_str
            context["paths.project_root"] = root_str
            context["workspace_root"] = root_str

            expanded = self._expand_tokens(merged, context)
            if expanded == merged:
                break
            merged = expanded

        # Validate that no unexpanded placeholders remain
        self._validate_no_placeholders(merged)
        return merged

    def _validate_no_placeholders(self, data: Any) -> None:
        pattern = re.compile(r"\{([\w\.]+)\}")
        if isinstance(data, str):
            unresolved = pattern.findall(data)
            if unresolved:
                raise ValueError(
                    f"Unresolved template placeholder(s) {unresolved} in value '{data}'. "
                    f"Check keys in CI_CD/config.yaml and CI_CD/config_ci.yaml."
                )
        elif isinstance(data, dict):
            for v in data.values():
                self._validate_no_placeholders(v)
        elif isinstance(data, list):
            for item in data:
                self._validate_no_placeholders(item)

    def run(self) -> None:
        stages: List[Dict[str, Any]] = self.cfg.get("pipeline", {}).get("stages", [])

        for stage in stages:
            name = stage.get("name", "unnamed").upper()
            stype = stage.get("type")
            print_stage(name)

            if stype == "host":
                self._execute_host_stage(stage)
            elif stype == "docker":
                self._execute_docker_stage(stage)
            elif stype == "artifact":
                self._execute_artifact_stage(stage)
            else:
                raise ValueError(f"Unknown stage type '{stype}' in stage '{name}'")

        print("\n✅ Pipeline Execution Completed Successfully\n")

    def _execute_host_stage(self, stage: Dict[str, Any]) -> None:
        for action in stage.get("actions", []):
            act_type = action.get("action")
            if act_type == "clean_path":
                self.docker_manager.cleanup_path(action.get("target", ""))
            elif act_type == "delete_file":
                f = Path(action.get("target", ""))
                if f.exists():
                    f.unlink(missing_ok=True)
            elif act_type == "git_checkout":
                dest = Path(action.get("dest", ""))
                git = GitManager(str(dest))
                git.clone(action.get("url"), branch=action.get("branch", "main"))
                git.init_submodules()

        for cmd in stage.get("commands", []):
            print(f"🚀 Host Executing: {cmd}")
            res = subprocess.run(cmd, shell=True, cwd=str(self.workspace_root))
            if res.returncode != 0:
                sys.exit(res.returncode)

    def _execute_docker_stage(self, stage: Dict[str, Any]) -> None:
        image = stage.get("image")
        workdir = Path(stage.get("workdir", str(self.workspace_root)))
        env_scripts = stage.get("env_scripts", [])
        pip_deps = stage.get("pip_dependencies", [])

        for cmd in stage.get("commands", []):
            self.docker_manager.run_container(
                image=image,
                command=cmd,
                workdir=workdir,
                env_scripts=env_scripts,
                pip_dependencies=pip_deps,
            )

    def _execute_artifact_stage(self, stage: Dict[str, Any]) -> None:
        for action in stage.get("actions", []):
            act_type = action.get("action")
            if act_type == "bundle":
                create_package(
                    source_dir=action.get("source_dir"),
                    dest_dir=action.get("dest_dir"),
                    includes=action.get("includes", ["*"]),
                )
                create_zip(
                    source_dir=action.get("dest_dir"),
                    zip_name=action.get("zip_output"),
                )
            elif act_type == "extract":
                archive = action.get("archive")
                dest = action.get("dest")
                if not Path(archive).exists():
                    raise FileNotFoundError(f"Archive not found: {archive}")
                extract_zip(archive, dest)