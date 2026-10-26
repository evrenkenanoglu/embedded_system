"""
@file       tasks.py
@brief      Invoke task definitions for the ota-tester tool.
@note       Exposes tasks under the 'tools.ota-tester.*' namespace.
"""

import sys
from pathlib import Path
from invoke import Context, task

TOOL_DIR = Path(__file__).resolve().parent
RUN_SCRIPT = TOOL_DIR / "run.py"


@task
def clean(c: Context) -> None:
    """Purge all release-catalog binaries, patches, and manifest.json."""
    cmd = f'"{sys.executable}" "{RUN_SCRIPT}" clean'
    c.run(cmd, pty=(sys.platform != "win32"))


@task(
    help={
        "version": "Baseline version string (default: 1.0.0)",
        "version_number": "Baseline integer version (default: 1)",
        "monitor": "Attach serial monitor after flashing baseline",
        "clean_catalog": "Clean release catalog before staging baseline (default: True)",
    }
)
def baseline(
    c: Context,
    version: str = "1.0.0",
    version_number: int = 1,
    monitor: bool = False,
    clean_catalog: bool = True,
) -> None:
    """Purge catalog, stage, build, provision, and flash baseline v1.0.0 firmware to board."""
    mon_flag = "--monitor" if monitor else ""
    clean_flag = "" if clean_catalog else "--no-clean"
    cmd = f'"{sys.executable}" "{RUN_SCRIPT}" baseline --version "{version}" --version-number {version_number} {mon_flag} {clean_flag}'
    c.run(cmd, pty=(sys.platform != "win32"))


@task(
    help={
        "version": "Target update version string (default: 1.0.1)",
        "version_number": "Target update integer version (default: 2)",
    }
)
def update(
    c: Context,
    version: str = "1.0.1",
    version_number: int = 2,
) -> None:
    """Stage, build, sign, diff, and publish v1.0.1 update without flashing hardware."""
    cmd = f'"{sys.executable}" "{RUN_SCRIPT}" update --version "{version}" --version-number {version_number}'
    c.run(cmd, pty=(sys.platform != "win32"))


@task(help={"monitor": "Attach serial monitor after flashing"})
def flash_chain(c: Context, monitor: bool = True) -> None:
    """Execute complete command chain: build-config -> build -> provision-nvs -> flash -> flash-nvs -> monitor."""
    mon_flag = "" if monitor else "--no-monitor"
    cmd = f'"{sys.executable}" "{RUN_SCRIPT}" flash-chain {mon_flag}'
    c.run(cmd, pty=(sys.platform != "win32"))


@task(
    help={
        "base_ver": "Baseline version (default: 1.0.0)",
        "base_num": "Baseline version number (default: 1)",
        "target_ver": "Target update version (default: 1.0.1)",
        "target_num": "Target update version number (default: 2)",
        "clean_catalog": "Purge release catalog before execution (default: True)",
    }
)
def e2e(
    c: Context,
    base_ver: str = "1.0.0",
    base_num: int = 1,
    target_ver: str = "1.0.1",
    target_num: int = 2,
    clean_catalog: bool = True,
) -> None:
    """Run full automated end-to-end OTA cycle: clean -> baseline flash -> update publish -> live monitor."""
    clean_flag = "" if clean_catalog else "--no-clean"
    cmd = (
        f'"{sys.executable}" "{RUN_SCRIPT}" e2e '
        f'--base-ver "{base_ver}" --base-num {base_num} '
        f'--target-ver "{target_ver}" --target-num {target_num} {clean_flag}'
    )
    c.run(cmd, pty=(sys.platform != "win32"))
