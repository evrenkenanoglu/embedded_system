"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for AI_Assistant_Runner.
@note       Completely independent of task-runner and core.
"""

import sys
from pathlib import Path
from invoke import Context, task


@task(
    help={
        "config": "Path to YAML configuration file (default: config.example.yaml)",
        "tool": "Active assistant CLI: copilot, claude, antigravity, generic",
        "provider": "Target provider backend: openrouter, ollama, groq, etc.",
        "opts": "Forward arbitrary extra flags directly to main.py",
        "dry_run": "Print execution command without running",
    }
)
def start(
    c: Context,
    config: str = "config.example.yaml",
    tool: str = "",
    provider: str = "",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Launch an interactive AI Assistant terminal session."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "main.py"

    config_path = Path(config)
    if not config_path.is_absolute() and not config_path.exists():
        config_path = (tool_dir / config_path).resolve()

    # Use sys.executable instead of hardcoded 'python'
    cmd_parts = [
        f'"{sys.executable}" "{script_path}"',
        f'"{config_path}"',
    ]
    if tool:
        cmd_parts.append(f'--tool "{tool.strip()}"')
    if provider:
        cmd_parts.append(f'--provider "{provider.strip()}"')

    cmd = " ".join(cmd_parts)
    if opts:
        cmd += f" {opts}"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"))


@task(
    help={
        "config": "Path to YAML configuration file (default: config.example.yaml)",
        "tool": "Target tool: copilot, claude, antigravity, generic",
        "provider": "Target provider: openrouter, ollama, groq, etc.",
        "format": "Script format: auto, sh, ps1, or both (default: auto)",
        "output_dir": "Target folder for generated script",
        "dry_run": "Print generation command without running",
    }
)
def generate(
    c: Context,
    config: str = "config.example.yaml",
    tool: str = "",
    provider: str = "",
    format: str = "auto",
    output_dir: str = "",
    dry_run: bool = False,
) -> None:
    """Generate standalone launch scripts (.sh / .ps1)."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "main.py"

    config_path = Path(config)
    if not config_path.is_absolute() and not config_path.exists():
        config_path = (tool_dir / config_path).resolve()

    # Use sys.executable instead of hardcoded 'python'
    cmd_parts = [
        f'"{sys.executable}" "{script_path}"',
        f'"{config_path}"',
        f'--generate "{format}"',
    ]
    if output_dir:
        cmd_parts.append(f'--output-dir "{output_dir}"')
    if tool:
        cmd_parts.append(f'--tool "{tool.strip()}"')
    if provider:
        cmd_parts.append(f'--provider "{provider.strip()}"')

    cmd = " ".join(cmd_parts)
    c.run(cmd, dry=dry_run)
