"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the release-catalog tool.
@note       Completely independent of task-runner and core.
"""

import sys
from pathlib import Path
from invoke import Context, task

TOOL_NAME = "release-catalog"


@task(
    help={
        "catalog_dir": "Path to the release-catalog directory (default: this folder)",
        "opts": "Forward arbitrary extra flags directly to diff_worker.py",
        "dry_run": "Print execution command without running",
    }
)
def reconcile(
    c: Context,
    catalog_dir: str = "",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Reconcile missing 1-hop delta patches across all catalog channels."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "diff_worker.py"

    catalog = Path(catalog_dir).resolve() if catalog_dir else tool_dir

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd = f'"{sys.executable}" "{script_path}" --catalog-dir "{catalog}" --reconcile'
    if opts:
        cmd += f" {opts}"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))


@task(
    help={
        "binary": "Path to the compiled application binary (.bin)",
        "version": "Firmware semver string (e.g. 1.0.1)",
        "signature": "Pre-computed developer target signature (HEX)",
        "signing_cert": "Path to developer signing.crt PEM",
        "hardware": "Target hardware profile",
        "channel": "Deployment cohort channel (stable, beta, testing, development)",
        "hsvn": "Hardware Security Version Number",
        "canary": "Canary rollout percentage (0-100)",
        "notes": "Changelog or release notes",
        "project_name": "Project name (default: derived by diff_worker.py)",
        "catalog_dir": "Path to the release-catalog directory (default: this folder)",
        "dry_run": "Print execution command without running",
    }
)
def register(
    c: Context,
    binary: str,
    version: str,
    signature: str = "",
    signing_cert: str = "",
    hardware: str = "ESP32-C6-WROOM-1",
    channel: str = "stable",
    hsvn: int = 1,
    canary: int = 100,
    notes: str = "Production release build.",
    project_name: str = "",
    catalog_dir: str = "",
    dry_run: bool = False,
) -> None:
    """Register a compiled binary release and generate its 1-hop delta patch."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "diff_worker.py"

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd_parts = [f'"{sys.executable}" "{script_path}"']
    if catalog_dir:
        cmd_parts.append(f'--catalog-dir "{Path(catalog_dir).resolve()}"')
    cmd_parts.extend(
        [
            f'--binary "{binary}"',
            f'--version "{version}"',
            f'--hardware "{hardware}"',
            f'--channel "{channel}"',
            f"--hsvn {hsvn}",
            f"--canary {canary}",
            f'--notes "{notes}"',
        ]
    )
    if signature:
        cmd_parts.append(f'--signature "{signature}"')
    if signing_cert:
        cmd_parts.append(f'--signing-cert "{Path(signing_cert).resolve()}"')
    if project_name:
        cmd_parts.append(f'--project-name "{project_name}"')

    c.run(" ".join(cmd_parts), dry=dry_run, pty=(sys.platform != "win32"))
