"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the AI Content Serializer tool.
@note       Completely independent of task-runner and core.
"""

import sys
from pathlib import Path
from invoke import Context, task

TOOL_NAME = "ai-content-serializer"


@task(
    help={
        "input_dirs": "Space-separated parent directories to scan (default: cwd)",
        "manifest": "Path to the JSON manifest output",
        "tree": "Path to the directory tree preview output",
        "output": "Path to the serialized Markdown output",
        "opts": "Forward arbitrary extra flags directly to runner.py",
        "dry_run": "Print execution command without running",
    }
)
def serialize(
    c: Context,
    input_dirs: str = "",
    manifest: str = "",
    tree: str = "",
    output: str = "",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Run the manifest generation + code context serialization pipeline."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "runner.py"

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd_parts = [f'"{sys.executable}" "{script_path}"']
    if input_dirs and str(input_dirs).strip():
        dirs = " ".join(f'"{d.strip()}"' for d in input_dirs.split() if d.strip())
        cmd_parts.append(f"--input_dirs {dirs}")
    if manifest and str(manifest).strip():
        cmd_parts.append(f'--manifest "{manifest}"')
    if tree and str(tree).strip():
        cmd_parts.append(f'--tree "{tree}"')
    if output and str(output).strip():
        cmd_parts.append(f'--output "{output}"')

    cmd = " ".join(cmd_parts)
    if opts:
        cmd += f" {opts}"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))
