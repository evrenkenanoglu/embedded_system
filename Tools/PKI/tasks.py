"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the PKI key generator tool.
@note       Completely independent of task-runner and core.
"""

import sys
from pathlib import Path
from invoke import Context, task

TOOL_NAME = "pki"


def _resolve_config_path(config: str, tool_dir: Path) -> Path:
    """Resolves a config path relative to the tool directory when needed."""
    config_path = Path(config)
    if not config_path.is_absolute() and not config_path.exists():
        config_path = (tool_dir / config_path).resolve()
    return config_path


@task(
    help={
        "config": "Path to PKI YAML configuration file (default: config.example.yaml)",
        "project_root": "Explicit project root directory override",
        "opts": "Forward arbitrary extra flags directly to generate_pki.py",
        "dry_run": "Print execution command without running",
    }
)
def generate(
    c: Context,
    config: str = "config.example.yaml",
    project_root: str = "",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Generate PKI certificates and silicon hardware keys via the standalone engine."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "generate_pki.py"
    config_path = _resolve_config_path(config, tool_dir)

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd_parts = [
        f'"{sys.executable}" "{script_path}"',
        f'--config="{config_path}"',
    ]
    if project_root:
        cmd_parts.append(f'--project-root="{project_root}"')

    cmd = " ".join(cmd_parts)
    if opts:
        cmd += f" {opts}"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))
