import os
import shutil
from pathlib import Path
from ..core.utils import run_cmd


class DockerManager:
    def __init__(self, project_root):
        self.project_root = Path(project_root).resolve()

    def _to_container_path(self, host_path) -> str:
        """Translates a host filesystem path to a POSIX path under /project."""
        resolved = Path(host_path).resolve()
        try:
            rel = resolved.relative_to(self.project_root)
            return f"/project/{rel.as_posix()}".rstrip("/")
        except ValueError:
            return f"/project/{resolved.name}"

    def run_container(self, image: str, command: str, workdir, volumes=None):
        container_workdir = self._to_container_path(workdir)
        host_mount = self.project_root.as_posix()

        cmd = [
            "docker",
            "run",
            "--rm",
            "-v",
            f"{host_mount}:/project",
            "-w",
            container_workdir,
        ]

        if volumes:
            for host_path, cont_path in volumes.items():
                posix_host = Path(host_path).resolve().as_posix()
                cmd.extend(["-v", f"{posix_host}:{cont_path}"])

        cmd.extend([image, "bash", "-c", command])

        print(f"🐳 Docker Executing: {command[:60]}...")
        return run_cmd(cmd)

    def cleanup_path(self, relative_path: str):
        """Cross-platform directory removal with fallback for container root-owned permissions."""
        if not relative_path or str(relative_path).strip() in ["/", ".", "./"]:
            print(f"⚠️ Cleanup blocked: Attempted to delete sensitive path '{relative_path}'")
            return

        target_path = (self.project_root / relative_path).resolve()
        if not target_path.exists():
            return

        print(f"🧹 Wiping directory: {relative_path}")

        # 1. Attempt host-level deletion
        try:
            def _handle_readonly(func, path, exc_info):
                import stat
                os.chmod(path, stat.S_IWRITE)
                func(path)

            shutil.rmtree(target_path, onerror=_handle_readonly)
        except PermissionError:
            # 2. Fallback to Alpine container if files were created with root permissions inside Docker
            posix_rel = Path(relative_path).as_posix().lstrip("/")
            cmd = [
                "docker",
                "run",
                "--rm",
                "-v",
                f"{self.project_root.as_posix()}:/project",
                "alpine",
                "sh",
                "-c",
                f"rm -rf /project/{posix_rel}",
            ]
            run_cmd(cmd)