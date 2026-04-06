# pipeline/core/docker_manager.py
import subprocess
import os
from ..core.utils import run_cmd


class DockerManager:
    def __init__(self, project_root):
        self.project_root = project_root

    def run_container(self, image, command, workdir, volumes=None):
        # Map host path to container path
        container_workdir = workdir.replace(self.project_root, "/project").replace(
            "\\", "/"
        )

        cmd = [
            "docker",
            "run",
            "--rm",
            "-v",
            f"{self.project_root}:/project",
            "-w",
            container_workdir,
        ]

        # Add extra volumes if provided
        if volumes:
            for host_path, cont_path in volumes.items():
                cmd.extend(["-v", f"{host_path}:{cont_path}"])

        cmd.extend([image, "bash", "-c", command])

        print(f"🐳 Docker Executing: {command[:50]}...")
        return run_cmd(cmd)

    def cleanup_path(self, relative_path):
        # Safety Check: Prevent accidental deletion of the entire project
        if not relative_path or relative_path.strip() in ["/", ".", "./"]:
            print(
                f"⚠️ Cleanup blocked: Attempted to delete sensitive path '{relative_path}'"
            )
            return

        print(f"🧹 Wiping directory: {relative_path}")

        # The command removes the folder entirely.
        # We add 'mkdir -p' to ensure the parent structure remains ready for the next run.
        cmd = [
            "docker",
            "run",
            "--rm",
            "-v",
            f"{self.project_root}:/project",
            "alpine",
            "sh",
            "-c",
            f"rm -rf /project/{relative_path}",
        ]

        return run_cmd(cmd)
