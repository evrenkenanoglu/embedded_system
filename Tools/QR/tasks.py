"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the generic QR code generator.
@note       Completely independent of task-runner and core. The payload is read from an
            environment variable (default QR_PAYLOAD) so it never appears in the command line.
"""

import sys
from pathlib import Path
from invoke import Context, Exit, task

TOOL_DIR = Path(__file__).resolve().parent


@task(
    help={
        "device_id": "Device identifier recorded in the traceability log (required)",
        "output": "PNG output path (required)",
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
    """Generate a QR image from the payload in an environment variable (runs main.py)."""
    if not device_id or not output or not log:
        raise Exit("--device-id, --output and --log are required", code=2)

    script_path = TOOL_DIR / "main.py"
    cmd = (
        f'"{sys.executable}" "{script_path}" --payload-env {payload_env}'
        f' --device-id "{device_id}" --output "{output}" --log "{log}"'
        f" --ecc {ecc} --scale {scale} --border {border}"
    )
    if svg:
        cmd += f' --svg "{svg}"'
    if force:
        cmd += " --force"
    c.run(cmd, dry=dry_run)
