"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the SSoT build artifacts generator tool.
@note       Completely independent of task-runner and core.
"""

import sys
from pathlib import Path
from invoke import Context, task

TOOL_NAME = "build-config"

TOOL_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOL_DIR))
from generate_artifacts import BUILD_TARGETS as VALID_TARGETS  # noqa: E402


@task(
    help={
        "config": "Path to resolved project config YAML (required)",
        "target": f"Artifact target: {', '.join(VALID_TARGETS)} (default: all)",
        "workspace": "Workspace root directory (default: auto-detected)",
        "opts": "Forward arbitrary extra flags directly to generate_artifacts.py",
        "dry_run": "Print execution command without running",
    }
)
def generate(
    c: Context,
    config: str = "",
    target: str = "all",
    workspace: str = "",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Generate partitions.csv, sdkconfig.hardware, project_version.cmake, and configured application headers."""
    if target not in VALID_TARGETS:
        raise ValueError(
            f"Invalid target '{target}'. Available options: {', '.join(VALID_TARGETS)}"
        )
    if not config or not str(config).strip():
        raise ValueError("--config is required (pass the resolved project config YAML)")

    script_path = TOOL_DIR / "generate_artifacts.py"
    config_path = Path(config)
    if not config_path.is_absolute() and not config_path.exists():
        config_path = (TOOL_DIR / config).resolve()

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd_parts = [
        f'"{sys.executable}" "{script_path}"',
        f'--config "{config_path}"',
        f"--target {target}",
    ]
    if workspace and str(workspace).strip():
        cmd_parts.append(f'--workspace "{workspace}"')

    cmd = " ".join(cmd_parts)
    if opts:
        cmd += f" {opts}"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))
