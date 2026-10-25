"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the multi-language code formatter tool.
@note       Completely independent of task-runner and core.
"""

import os
import sys
from pathlib import Path
from invoke import Context, task

TOOL_NAME = "formatter"

VALID_LANGS = ("all", "c", "python", "yaml", "cmake")
VALID_MODES = ("apply", "check")
VALID_SCOPES = ("changed", "all")


def _execution_env() -> dict:
    """Prepends candidate interpreter bin dirs to PATH so console scripts resolve.

    Prefers the canonical project venv (.venv/bin) when present, guaranteeing the
    formatter subprocess sees the venv's dependencies (pathspec, black, etc.)
    even when the hook is launched by a bare system interpreter.
    """
    env = dict(os.environ)
    candidates = []
    venv_dir = Path(__file__).resolve()
    while venv_dir != venv_dir.parent:
        venv_dir = venv_dir.parent
        candidate = venv_dir / ".venv" / ("Scripts" if sys.platform == "win32" else "bin")
        if (candidate / "activate").exists() or candidate.is_dir():
            candidates.append(str(candidate))
            break
    candidates.append(str(Path(sys.executable).parent))
    env["PATH"] = os.pathsep.join([*candidates, env.get("PATH", "")])
    return env


def _resolve_venv_python() -> str:
    """Returns the project venv interpreter path if available, else sys.executable."""
    marker = "python.exe" if sys.platform == "win32" else "python"
    probe = Path(__file__).resolve()
    while probe != probe.parent:
        probe = probe.parent
        candidate = probe / ".venv" / ("Scripts" if sys.platform == "win32" else "bin") / marker
        if candidate.exists():
            return str(candidate)
    return sys.executable


def _resolve_config_path(config: str, tool_dir: Path) -> Path:
    """Resolves a config path relative to the tool directory when needed."""
    config_path = Path(config)
    if not config_path.is_absolute() and not config_path.exists():
        config_path = (tool_dir / config_path).resolve()
    return config_path


@task(
    help={
        "config": "Path to resolved formatter YAML config (default: config.example.yaml)",
        "lang": f"Target language: {', '.join(VALID_LANGS)} (default: all)",
        "mode": f"Action mode: {', '.join(VALID_MODES)} (default: apply)",
        "scope": f"File scope: {', '.join(VALID_SCOPES)} (default: changed)",
        "c_style_config": "Optional override path for C/C++ style config (.clang-format)",
        "python_style_config": "Optional override path for Python style config (pyproject.toml)",
        "opts": "Forward arbitrary extra flags directly to main.py",
        "dry_run": "Print execution command without running",
    }
)
def format(
    c: Context,
    config: str = "config.example.yaml",
    lang: str = "all",
    mode: str = "apply",
    scope: str = "changed",
    c_style_config: str = "",
    python_style_config: str = "",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Run the multi-language code formatter (C, Python, YAML, CMake)."""
    if lang not in VALID_LANGS:
        raise ValueError(
            f"Invalid lang '{lang}'. Available options: {', '.join(VALID_LANGS)}"
        )
    if mode not in VALID_MODES:
        raise ValueError(
            f"Invalid mode '{mode}'. Available options: {', '.join(VALID_MODES)}"
        )
    if scope not in VALID_SCOPES:
        raise ValueError(
            f"Invalid scope '{scope}'. Available options: {', '.join(VALID_SCOPES)}"
        )

    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "main.py"
    config_path = _resolve_config_path(config, tool_dir)

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd_parts = [
        f'"{_resolve_venv_python()}" "{script_path}"',
        f'--config="{config_path}"',
        f"--lang {lang}",
        f"--mode {mode}",
        f"--scope {scope}",
    ]
    if c_style_config:
        cmd_parts.append(f'--c-style-config "{c_style_config}"')
    if python_style_config:
        cmd_parts.append(f'--python-style-config "{python_style_config}"')

    cmd = " ".join(cmd_parts)
    if opts:
        cmd += f" {opts}"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"), env=_execution_env())
