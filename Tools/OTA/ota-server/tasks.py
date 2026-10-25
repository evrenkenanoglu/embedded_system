"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the OTA server tool.
@note       Completely independent of task-runner and core.
"""

import sys
from pathlib import Path
from invoke import Context, task

TOOL_NAME = "ota-server"


def _resolve_config_path(config: str, tool_dir: Path) -> Path:
    """Resolves a config path relative to the tool directory when needed."""
    config_path = Path(config)
    if not config_path.is_absolute() and not config_path.exists():
        config_path = (tool_dir / config_path).resolve()
    return config_path


@task(
    help={
        "config": "Path to YAML configuration file (default: config.example.yaml)",
        "opts": "Forward arbitrary extra flags directly to run.py",
        "dry_run": "Print execution command without running",
    }
)
def start(
    c: Context,
    config: str = "config.example.yaml",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Launch the secure OTA server standalone engine."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "run.py"
    config_path = _resolve_config_path(config, tool_dir)

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd = f'"{sys.executable}" "{script_path}" --config="{config_path}"'
    if opts:
        cmd += f" {opts}"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))


@task(
    help={
        "config": "Path to SSoT YAML configuration file (default: config.example.yaml)",
        "channel": "Deployment channel target (stable, beta, testing, development)",
        "notes": "Descriptive change log or release notes",
        "url": "Destination upload URL endpoint (default: derived from config)",
        "insecure": "Bypass SSL certificate verification (local self-signed testing)",
        "dry_run": "Print execution command without running",
    }
)
def upload(
    c: Context,
    config: str = "config.example.yaml",
    channel: str = "stable",
    notes: str = "Automated release pipeline upload.",
    url: str = "",
    insecure: bool = True,
    dry_run: bool = False,
) -> None:
    """Upload compiled firmware to the OTA server via HTTP API."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "upload_firmware.py"
    config_path = _resolve_config_path(config, tool_dir)

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd_parts = [
        f'"{sys.executable}" "{script_path}"',
        f'--config="{config_path}"',
        f'--channel="{channel}"',
        f'--notes="{notes}"',
    ]
    if url:
        cmd_parts.append(f'--url="{url}"')
    if insecure:
        cmd_parts.append("--insecure")

    cmd = " ".join(cmd_parts)
    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))
