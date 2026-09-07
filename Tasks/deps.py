from pathlib import Path
from invoke import Context, task
from core import CONFIG, IS_WINDOWS, CommandSerializer

@task(
    help={
        "upgrade": "Re-resolve all dependencies to latest versions",
    }
)
def compile(c: Context, upgrade: bool = False) -> None:
    """Compile cross-platform requirements.in to requirements.txt using uv."""
    in_path = CONFIG.paths.embedded_requirements_in
    out_path = CONFIG.paths.embedded_requirements_file

    if not in_path.exists():
        raise FileNotFoundError(f"Requirements file not found: {in_path}")

    upgrade_flag = "--upgrade" if upgrade else ""
    rel_in = in_path.relative_to(CONFIG.paths.embedded_system_dir)
    rel_out = out_path.relative_to(CONFIG.paths.embedded_system_dir)

    cmd = (
        f'cd "{CONFIG.paths.embedded_system_dir}" && '
        f'uv pip compile "{rel_in}" '
        f'-o "{rel_out}" '
        f'--universal '
        f'--annotation-style line '
        f'{upgrade_flag}'
    ).strip()

    serializer = CommandSerializer()
    serializer.add(cmd)
    serializer.run(c)

@task
def install(c: Context, upgrade: bool = False) -> None:
    """Install embedded system dependencies into .venv."""
    req_path = CONFIG.paths.embedded_requirements_file
    if not req_path.exists():
        raise FileNotFoundError(f"Requirements lockfile not found: {req_path}")

    upgrade_flag = "--upgrade" if upgrade else ""
    python_bin = CONFIG.paths.venv_bin_dir / ("python.exe" if IS_WINDOWS else "python")

    cmd = f'uv pip install -r "{req_path}" --python "{python_bin}" {upgrade_flag}'.strip()
    serializer = CommandSerializer()
    serializer.add(cmd)
    serializer.run(c)