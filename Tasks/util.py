from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer

@task(
    help={
        "dry_run": "Print execution command without running",
        "opts": "Forward arbitrary arguments to the system file generator script",
    }
)
def system_file_generator(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Run the System Source Files Generator script."""
    script_path = CONFIG.paths.system_file_generator_script
    if not script_path.exists():
        raise FileNotFoundError(f"System file generator script not found: {script_path}")

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(f'python "{script_path}"', extra=opts)
    serializer.run(c, dry_run=dry_run)