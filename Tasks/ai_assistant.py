"""
@file       ai_assistant.py
@brief      AI Assistant orchestration task-runner module for interactive CLI sessions.
@copyright  (c) 2026 - All Rights Reserved
"""

from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer
from core.config_resolver import resolve_to_file


@task(
    help={
        "tool": "Active assistant to launch: copilot, claude, antigravity, generic (defaults to SSoT active_tool)",
        "config": "Explicit path to custom AI assistant configuration file (overrides SSoT)",
        "dry_run": "Print execution command without running",
        "opts": "Forward arbitrary extra flags directly to the assistant CLI",
    }
)
def start(
    c: Context,
    tool: str = "",
    config: str = "",
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Launch an interactive AI Assistant terminal session configured via SSoT."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve Script Path strictly from SSoT
    raw_script = getattr(CONFIG.paths, "ai_assistant_runner_script", None)
    if not raw_script:
        raise KeyError(
            "SSoT Error: 'paths.ai_assistant_runner_script' is not declared in config_tasks.yaml."
        )

    script_path = Path(raw_script).resolve()
    if not script_path.exists():
        raise FileNotFoundError(
            f"AI Assistant runner script not found at SSoT resolved path: {script_path}"
        )

    # 2. Resolve Config Path from SSoT or CLI override
    if config and str(config).strip():
        raw_config = Path(config)
    else:
        raw_config_path = (
            getattr(CONFIG.paths, "es_config_ai_assistant", None)
            or getattr(CONFIG.paths, "config_ai_assistant", None)
            or (Path(CONFIG.paths.configs_dir) / "config_ai_assistant.yaml")
        )
        raw_config = Path(raw_config_path)

    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config_file = resolve_to_file(raw_config, workspace_root=workspace_root)

    # 3. Construct Command
    cmd_parts = [
        f'python "{script_path}"',
        f'"{resolved_config_file}"',
    ]

    if tool and str(tool).strip():
        cmd_parts.append(f'--tool "{tool.strip()}"')

    cmd = " ".join(cmd_parts)

    # 4. Serialize and Execute via Subprocess
    execution_env = dict(CONFIG.env or {})

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=execution_env,
    )
    serializer.add(cmd, extra=opts)

    print("\n[*] [AI ASSISTANT] Launching interactive assistant session...")
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "tool": "Target tool: copilot, claude, antigravity, generic (defaults to SSoT active_tool)",
        "format": "Script format: auto, sh, ps1, or both (default: auto-detects host OS)",
        "output_dir": "Target folder for generated script (defaults to {paths.output_dir}/scripts)",
        "config": "Explicit path to custom configuration file",
        "dry_run": "Print generation command without running",
    }
)
def generate(
    c: Context,
    tool: str = "",
    format: str = "auto",
    output_dir: str = "",
    config: str = "",
    dry_run: bool = False,
) -> None:
    """Generate standalone Bash (.sh) or PowerShell (.ps1) launch scripts from SSoT."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve Script Path from SSoT
    raw_script = getattr(CONFIG.paths, "ai_assistant_runner_script", None)
    if not raw_script:
        raise KeyError(
            "SSoT Error: 'paths.ai_assistant_runner_script' is not declared."
        )
    script_path = Path(raw_script).resolve()

    # 2. Resolve Config Path from SSoT
    if config and str(config).strip():
        raw_config = Path(config)
    else:
        raw_config_path = (
            getattr(CONFIG.paths, "es_config_ai_assistant", None)
            or getattr(CONFIG.paths, "config_ai_assistant", None)
            or (Path(CONFIG.paths.configs_dir) / "config_ai_assistant.yaml")
        )
        raw_config = Path(raw_config_path)

    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config_file = resolve_to_file(raw_config, workspace_root=workspace_root)

    # 3. Determine Output Directory (Defaults to SSoT output_dir/scripts)
    if not output_dir:
        output_dir = str(Path(CONFIG.paths.output_dir) / "scripts")

    cmd = (
        f'python "{script_path}" "{resolved_config_file}" '
        f'--generate "{format}" --output-dir "{output_dir}"'
    )
    if tool and str(tool).strip():
        cmd += f' --tool "{tool.strip()}"'

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=dict(CONFIG.env or {}),
    )
    serializer.add(cmd)
    serializer.run(c, dry_run=dry_run)
