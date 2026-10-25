"""
@file       wsl.py
@brief      WSL USB bridge orchestration task-runner module.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import sys
from invoke import Context, task


@task(
    help={
        "dry_run": "Print execution command without running",
        "opts": "Forward arbitrary extra flags to the WSL tool",
    }
)
def usb_bridge(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Run the WSL Smart Attach/Detach cycle for USB devices (detach + reattach)."""
    opts_flag = f'--opts="{opts.strip()}"' if opts and str(opts).strip() else ""
    c.run(
        f'"{sys.executable}" -m invoke tools.wsl.reattach {opts_flag}'.strip(),
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )


@task(
    help={
        "dry_run": "Print execution command without running",
        "opts": "Forward arbitrary extra flags to the WSL tool",
    }
)
def usb_bridge_daemon(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Run the WSL Smart Auto Attach Daemon for USB devices."""
    opts_flag = f'--opts="{opts.strip()}"' if opts and str(opts).strip() else ""
    c.run(
        f'"{sys.executable}" -m invoke tools.wsl.daemon {opts_flag}'.strip(),
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )
