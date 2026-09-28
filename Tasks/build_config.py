"""
@file       build_config.py
@brief      Invoke tasks for SSoT build configuration generators using config_resolver.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer
from core.config_resolver import resolve_to_file


def _get_script_path() -> Path:
    script_path = Path(
        getattr(
            CONFIG.paths,
            "generator_artifacts_script",
            Path(CONFIG.paths.embedded_system_dir)
            / "Tools"
            / "BUILD_CONFIG"
            / "generate_artifacts.py",
        )
    ).resolve()

    if not script_path.exists():
        raise FileNotFoundError(f"Generator tool script missing at: {script_path}")

    return script_path


def _run_generator(
    c: Context, target: str, dry_run: bool, config_override: str = ""
) -> None:
    script_path = _get_script_path()
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve raw input config path (defaults to configs/config_project.yaml)
    raw_config = (
        Path(config_override)
        if config_override and str(config_override).strip()
        else workspace_root / "configs" / "config_project.yaml"
    )
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    # 2. Compile into flat, standalone YAML in build/configs/
    resolved_config_file = resolve_to_file(
        raw_config, workspace_root=str(workspace_root)
    )

    # 3. Construct and dispatch command
    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    cmd = (
        f'python "{script_path}" '
        f'--config "{resolved_config_file}" '
        f'--workspace "{workspace_root}" '
        f'--target {target}'
    )
    serializer.add(cmd)
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def partitions(c: Context, dry_run: bool = False, config: str = "") -> None:
    """Generate partitions.csv from Master SSoT."""
    _run_generator(c, "partitions", dry_run=dry_run, config_override=config)


@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def sdkconfig(c: Context, dry_run: bool = False, config: str = "") -> None:
    """Generate sdkconfig.hardware overlay from Master SSoT."""
    _run_generator(c, "sdkconfig", dry_run=dry_run, config_override=config)


@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def version(c: Context, dry_run: bool = False, config: str = "") -> None:
    """Generate project_version.cmake from Master SSoT."""
    _run_generator(c, "version", dry_run=dry_run, config_override=config)


@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def ota_header(c: Context, dry_run: bool = False, config: str = "") -> None:
    """Generate main/swConfig/ota_generated_config.h from Master SSoT."""
    _run_generator(c, "ota_header", dry_run=dry_run, config_override=config)


@task(
    name="all",
    default=True,
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom project config (defaults to configs/config_project.yaml)",
    },
)
def generate(c: Context, dry_run: bool = False, config: str = "") -> None:
    """Generate partitions.csv, sdkconfig.hardware, project_version.cmake, and ota_generated_config.h."""
    _run_generator(c, "all", dry_run=dry_run, config_override=config)
