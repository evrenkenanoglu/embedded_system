"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the per-device SoftAP password tool.
@note       Completely independent of task-runner and core.
"""

import sys
from pathlib import Path
from invoke import Context, task

TOOL_NAME = "ap-password"


@task(
    help={
        "device_id": "Device identifier stored in the provisioning record (required)",
        "output": "Path of the JSON record to create (required, outside the workspace)",
        "length": "Password length, 8..63 (default: 16)",
        "force": "Overwrite an existing record",
        "dry_run": "Print execution command without running",
    }
)
def generate(
    c: Context,
    device_id: str = "",
    output: str = "",
    length: int = 16,
    force: bool = False,
    dry_run: bool = False,
) -> None:
    """Generate a per-device SoftAP password and write its JSON provisioning record."""
    if not device_id.strip() or not output.strip():
        raise ValueError("Both --device-id and --output are required")

    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "main.py"

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd = (
        f'"{sys.executable}" "{script_path}" '
        f'--device-id "{device_id}" --output "{output}" --length {int(length)}'
    )
    if force:
        cmd += " --force"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))
