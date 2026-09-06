from invoke import Context, task
from core import CONFIG, CommandSerializer

@task
def server(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Start the Embedded System OTA server."""
    script_path = CONFIG.paths.ota_server_script
    if not script_path.exists():
        raise FileNotFoundError(f"OTA server script not found: {script_path}")

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(f'python "{script_path}"', extra=opts)
    serializer.run(c, dry_run=dry_run)