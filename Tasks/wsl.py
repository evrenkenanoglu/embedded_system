from invoke import Context, task
from core import CONFIG, CommandSerializer

@task
def usb_bridge(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Run the WSL Smart Attach/Detach script for USB devices."""
    script_path = CONFIG.paths.smart_deattach_attach_script
    if not script_path.exists():
        raise FileNotFoundError(f"WSL script not found: {script_path}")

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(f'python "{script_path}"', extra=opts)
    serializer.run(c, dry_run=dry_run)