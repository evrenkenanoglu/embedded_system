from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer


def _run_pipeline_script(
    c: Context,
    script_name: str,
    config: str = "",
    dry_run: bool = False,
    opts: str = "",
) -> None:
    workspace_dir = Path(getattr(CONFIG.paths, "workspace_dir", ".")).resolve()
    script_path = workspace_dir / "CI_CD" / script_name

    if not script_path.exists():
        raise FileNotFoundError(f"Pipeline script not found at: {script_path}")

    # Resolve config file: task argument override -> default CI_CD/config.yaml
    default_config = workspace_dir / "CI_CD" / "config.yaml"
    target_config = config if config and str(config).strip() else str(default_config)

    config_arg = f'--config "{target_config}"' if target_config else ""
    cmd = f'python "{script_path}" {config_arg}'.strip()

    serializer = CommandSerializer(
        prefix_commands=[getattr(CONFIG, "venv_activate_cmd", "")],
        env=getattr(CONFIG, "env", None),
    )
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "config": "Path to YAML configuration file (defaults to CI_CD/config.yaml)",
        "dry_run": "Print execution command without running",
        "opts": "Forward arbitrary arguments to run_ci.py",
    }
)
def ci(
    c: Context,
    config: str = "",
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Run the CI build, containerized compilation, and artifact packaging pipeline."""
    _run_pipeline_script(c, "run_ci.py", config=config, dry_run=dry_run, opts=opts)


@task(
    help={
        "config": "Path to YAML configuration file (defaults to CI_CD/config.yaml)",
        "dry_run": "Print execution command without running",
        "opts": "Forward arbitrary arguments to run_hil.py",
    }
)
def hil(
    c: Context,
    config: str = "",
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Run artifact unpacking, physical hardware flashing, and HIL test suite."""
    _run_pipeline_script(c, "run_hil.py", config=config, dry_run=dry_run, opts=opts)