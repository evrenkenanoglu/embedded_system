"""
@file       qr.py
@brief      Invoke adapter for the generic QR generator. Delegates to the standalone tool task.
@note       Payload is never passed on the command line. The caller exports it into the
            environment variable named by payload_env, and the tool reads it from there.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import os
import sys
from pathlib import Path

from invoke import Context, Exit, task
from core import CONFIG


def _resolve(path_str: str, workspace_root: Path) -> str:
    path = Path(path_str)
    return str(path if path.is_absolute() else (workspace_root / path).resolve())


@task(
    help={
        "device_id": "Device identifier recorded in the traceability log (required)",
        "output": "PNG output path; relative paths resolve from the workspace root (required)",
        "log": "Append-only JSON Lines traceability log path (required)",
        "svg": "Optional SVG output path",
        "ecc": "Error correction level: L, M, Q or H (default: M)",
        "scale": "Pixels per module (default: 10)",
        "border": "Quiet-zone width in modules (default: 4)",
        "payload_env": "Environment variable holding the payload (default: QR_PAYLOAD)",
        "force": "Overwrite existing output files",
        "dry_run": "Print execution command without running",
    }
)
def generate(
    c: Context,
    device_id: str = "",
    output: str = "",
    log: str = "",
    svg: str = "",
    ecc: str = "M",
    scale: int = 10,
    border: int = 4,
    payload_env: str = "QR_PAYLOAD",
    force: bool = False,
    dry_run: bool = False,
) -> None:
    """Generate a QR image from the payload exported in payload_env (delegates to tools.qr.generate)."""
    if not device_id or not output or not log:
        raise Exit("--device-id, --output and --log are required", code=2)
    if not os.environ.get(payload_env):
        raise Exit(
            f"payload environment variable {payload_env} is not set or empty", code=2
        )

    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()
    cmd = (
        f'"{sys.executable}" -m invoke tools.qr.generate'
        f' --device-id="{device_id}"'
        f' --output="{_resolve(output, workspace_root)}"'
        f' --log="{_resolve(log, workspace_root)}"'
        f" --ecc={ecc} --scale={scale} --border={border}"
        f" --payload-env={payload_env}"
    )
    if svg:
        cmd += f' --svg="{_resolve(svg, workspace_root)}"'
    if force:
        cmd += " --force"
    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))
