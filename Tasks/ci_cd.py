import sys
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
    script_path = (workspace_dir / "CI_CD" / script_name).resolve()

    if not script_path.exists():
        raise FileNotFoundError(f"Pipeline script not found at: {script_path}")

    default_config = workspace_dir / "CI_CD" / "config.yaml"
    target_config = config if config and str(config).strip() else str(default_config)

    # Use sys.executable for cross-platform interpreter invocation
    python_bin = Path(sys.executable).as_posix()
    script_posix = script_path.as_posix()
    config_posix = Path(target_config).as_posix()

    cmd = f'"{python_bin}" "{script_posix}" --config "{config_posix}"'

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