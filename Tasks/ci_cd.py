from pathlib import Path
from invoke import Context, task
from core import CONFIG
from core.config_resolver import resolve_to_file
from embedded_system.CI_CD.pipeline.core.engine import PipelineEngine


def _resolve_config_path(
    config_arg: str, default_filename: str, workspace_root: Path
) -> Path:
    """Resolves config path from CLI argument, configured CI/CD directory, or workspace root."""
    ci_cd_dir = Path(getattr(CONFIG.paths, "ci_cd_dir", workspace_root / "CI_CD"))

    # 1. Fallback to default if no argument is supplied
    if not config_arg or not str(config_arg).strip():
        return (ci_cd_dir / default_filename).resolve()

    target_path = Path(config_arg)

    # 2. Handle absolute paths directly
    if target_path.is_absolute():
        return target_path.resolve()

    # 3. Check relative to workspace root (e.g., --config=CI_CD/custom_ci.yaml)
    candidate_ws = (workspace_root / target_path).resolve()
    if candidate_ws.exists():
        return candidate_ws

    # 4. Check relative to CI/CD directory (e.g., --config=custom_ci.yaml)
    return (ci_cd_dir / target_path).resolve()


def _run_engine(c: Context, config_arg: str, default_name: str, dry_run: bool) -> None:
    workspace_root = Path(getattr(CONFIG.paths, "workspace_dir", ".")).resolve()
    raw_config_path = _resolve_config_path(config_arg, default_name, workspace_root)

    if not raw_config_path.exists():
        raise FileNotFoundError(f"Pipeline config not found: {raw_config_path}")

    # Compile SSoT inheritance (base_config) and tokens into flat YAML in build/configs/
    resolved_config_path = resolve_to_file(
        raw_config_path, workspace_root=workspace_root
    )

    if dry_run:
        print(
            f"[DRY-RUN] Would execute engine using configuration: {resolved_config_path}"
        )
        return

    engine = PipelineEngine(
        config_path=resolved_config_path, workspace_root=workspace_root
    )
    engine.run()


@task(
    help={
        "config": "Path or filename of CI configuration (defaults to CI_CD/config_ci.yaml)",
        "dry_run": "Print configuration path without executing",
    }
)
def ci(c: Context, config: str = "", dry_run: bool = False) -> None:
    """Execute CI build and artifact packaging pipeline."""
    _run_engine(c, config_arg=config, default_name="config_ci.yaml", dry_run=dry_run)


@task(
    help={
        "config": "Path or filename of HIL configuration (defaults to CI_CD/config_hil.yaml)",
        "dry_run": "Print configuration path without executing",
    }
)
def hil(c: Context, config: str = "", dry_run: bool = False) -> None:
    """Execute hardware flashing and HIL test suite."""
    _run_engine(c, config_arg=config, default_name="config_hil.yaml", dry_run=dry_run)
