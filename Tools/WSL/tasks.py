"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the WSL USB bridge tools.
@note       Completely independent of task-runner and core.
"""

import sys
from pathlib import Path
from invoke import Context, task

TOOL_NAME = "wsl"


def _script_cmd(name: str) -> str:
    """Builds a quoted sys.executable invocation for a sibling script."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / name
    return f'"{sys.executable}" "{script_path}"'


@task(
    help={
        "opts": "Forward arbitrary extra flags to the script",
        "dry_run": "Print execution command without running",
    }
)
def attach(
    c: Context,
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Find the ESP32/serial device and attach it to WSL2 (self-elevates on Windows)."""
    cmd = _script_cmd("Smart_Attach.py")
    if opts:
        cmd += f" {opts}"
    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))


@task(
    help={
        "opts": "Forward arbitrary extra flags to the script",
        "dry_run": "Print execution command without running",
    }
)
def deattach(
    c: Context,
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Detach the ESP32/serial device from WSL2 back to Windows."""
    cmd = _script_cmd("Smart_Deattach.py")
    if opts:
        cmd += f" {opts}"
    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))


@task(
    help={
        "opts": "Forward arbitrary extra flags to the script",
        "dry_run": "Print execution command without running",
    }
)
def reattach(
    c: Context,
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Power-cycle the device: detach from WSL, wait, then reattach."""
    cmd = _script_cmd("Smart_Deattach_Attach.py")
    if opts:
        cmd += f" {opts}"
    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))


@task(
    help={
        "opts": "Forward arbitrary extra flags to the script",
        "dry_run": "Print execution command without running",
    }
)
def daemon(
    c: Context,
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Run the auto-attach daemon that re-binds devices automatically."""
    cmd = _script_cmd("Smart_Auto_Attach_Daemon.py")
    if opts:
        cmd += f" {opts}"
    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))
