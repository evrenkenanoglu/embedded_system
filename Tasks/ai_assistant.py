"""
@file       ai_assistant.py
@brief      AI Assistant orchestration task-runner module for interactive CLI sessions.
@copyright  (c) 2026 - All Rights Reserved
"""

import sys
from pathlib import Path
from invoke import Context, task
from core import CONFIG
from core.config_resolver import resolve_to_file


@task(
    help={
        "tool": "Target assistant CLI: copilot, claude, antigravity, generic (default: copilot)",
        "provider": "Backend provider: openrouter, ollama, groq, etc.",
        "config_path": "Explicit path to custom configuration YAML (overrides SSoT)",
        "dry_run": "Print execution command without running",
    }
)
def start(
    c: Context,
    tool: str = "copilot",
    provider: str = "",
    config_path: str = "",
    dry_run: bool = False,
) -> None:
    """Resolve SSoT config and delegate to the tool's start task."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve SSoT config at this level
    raw_config = (
        Path(config_path)
        if config_path
        else Path(CONFIG.paths.configs_dir) / "config_ai_assistant.yaml"
    )
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config = resolve_to_file(raw_config, workspace_root=workspace_root)

    # 2. Delegate to the tool's runtime task (zero file script paths)
    provider_flag = f'--provider="{provider.strip()}"' if provider else ""
    c.run(
        f'inv tools.ai-assistant-runner.start --config="{resolved_config}" --tool="{tool}" {provider_flag}',
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )


@task(
    help={
        "tool": "Target assistant CLI: copilot, claude, antigravity, generic (default: copilot)",
        "provider": "Backend provider: openrouter, ollama, groq, etc.",
        "format": "Script format: auto, sh, ps1, or both (default: auto)",
        "output_dir": "Output directory for generated scripts",
        "config_path": "Explicit path to custom configuration YAML (overrides SSoT)",
        "dry_run": "Print execution command without running",
    }
)
def generate(
    c: Context,
    tool: str = "copilot",
    provider: str = "",
    format: str = "auto",
    output_dir: str = "",
    config_path: str = "",
    dry_run: bool = False,
) -> None:
    """Resolve SSoT config and delegate to the tool's generate task."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve SSoT config at this level
    raw_config = (
        Path(config_path)
        if config_path
        else Path(CONFIG.paths.configs_dir) / "config_ai_assistant.yaml"
    )
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config = resolve_to_file(raw_config, workspace_root=workspace_root)
    out = output_dir if output_dir else str(Path(CONFIG.paths.output_dir) / "scripts")

    # 2. Delegate to the tool's runtime task
    provider_flag = f'--provider="{provider.strip()}"' if provider else ""
    c.run(
        f'inv tools.ai-assistant-runner.generate --config="{resolved_config}" --tool="{tool}" --format="{format}" --output-dir="{out}" {provider_flag}',
        dry=dry_run,
    )
