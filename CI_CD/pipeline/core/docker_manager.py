import os
import platform
import shutil
from pathlib import Path
from typing import Dict, List, Optional
from ..core.utils import run_cmd

IS_WINDOWS = platform.system() == "Windows"


class DockerManager:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root).resolve()

    def _to_container_path(self, host_path) -> str:
        """Translates host path to POSIX container path under /project."""
        resolved = Path(host_path).resolve()
        try:
            rel = resolved.relative_to(self.project_root)
            return f"/project/{rel.as_posix()}".rstrip("/")
        except ValueError:
            return f"/project/{resolved.name}"

    def _get_host_pip_cache_dir(self) -> Path:
        """Resolves host pip cache across Windows, WSL, and Linux."""
        if IS_WINDOWS:
            cache = Path(os.environ.get("LOCALAPPDATA", "~/AppData/Local")).expanduser() / "pip" / "cache"
        else:
            cache = Path(os.environ.get("XDG_CACHE_HOME", "~/.cache")).expanduser() / "pip"
        cache.mkdir(parents=True, exist_ok=True)
        return cache.resolve()

    def _build_shell_preamble(
        self,
        container_workdir: str,
        env_scripts: Optional[List[str]] = None,
        pip_dependencies: Optional[List[str]] = None,
    ) -> str:
        """Constructs safe initialization steps inside the container before executing the payload."""
        steps = ["set -eo pipefail"]

        # 1. Enforce working directory (overrides Docker image entrypoint overrides)
        steps.append(f"cd '{container_workdir}'")

        # 2. Source environment scripts (e.g., Matter / ESP-IDF)
        for script in env_scripts or []:
            steps.append(f"[ -f '{script}' ] && source '{script}'")

        # 3. Verify Python packages via import check before calling pip
        if pip_dependencies:
            import_modules = [pkg.replace("-", "_").split("=")[0].split(">")[0].split("<")[0] for pkg in pip_dependencies]
            import_check = f"python3 -c 'import {', '.join(import_modules)}' >/dev/null 2>&1"
            install_cmd = f"pip install --no-cache-dir {' '.join(pip_dependencies)}"
            steps.append(f"{import_check} || {install_cmd}")

        return " && ".join(steps)

    def run_container(
        self,
        image: str,
        command: str,
        workdir: Path,
        env_scripts: Optional[List[str]] = None,
        pip_dependencies: Optional[List[str]] = None,
        volumes: Optional[Dict[str, str]] = None,
    ):
        """Executes a command in Docker with automatic environment initialization and pip caching."""
        container_workdir = self._to_container_path(workdir)
        host_mount = self.project_root.as_posix()
        pip_cache_host = self._get_host_pip_cache_dir().as_posix()

        cmd = [
            "docker",
            "run",
            "--rm",
            "-v", f"{host_mount}:/project",
            "-v", f"{pip_cache_host}:/root/.cache/pip",
            "-w", container_workdir,
        ]

        if volumes:
            for host_path, cont_path in volumes.items():
                posix_host = Path(host_path).resolve().as_posix()
                cmd.extend(["-v", f"{posix_host}:{cont_path}"])

        # Combine bootstrap preamble with payload command
        preamble = self._build_shell_preamble(container_workdir, env_scripts, pip_dependencies)
        full_command = f"{preamble} && {command}"

        cmd.extend([image, "bash", "-c", full_command])

        print(f"🐳 Docker Executing: {command} (in {container_workdir})")
        return run_cmd(cmd)

    def cleanup_path(self, relative_path: str) -> None:
        """Cross-platform directory removal with fallback for container root-owned files."""
        if not relative_path or str(relative_path).strip() in ["/", ".", "./"]:
            print(f"⚠️ Cleanup blocked: Attempted to delete protected root path '{relative_path}'")
            return

        target_path = (self.project_root / relative_path).resolve()
        if not target_path.exists():
            return

        print(f"🧹 Wiping directory: {target_path}")

        try:
            def _handle_readonly(func, path, exc_info):
                import stat
                os.chmod(path, stat.S_IWRITE)
                func(path)

            shutil.rmtree(target_path, onerror=_handle_readonly)
        except PermissionError:
            posix_rel = Path(relative_path).as_posix().lstrip("/")
            cmd = [
                "docker",
                "run",
                "--rm",
                "-v", f"{self.project_root.as_posix()}:/project",
                "alpine",
                "sh",
                "-c", f"rm -rf /project/{posix_rel}",
            ]
            run_cmd(cmd)