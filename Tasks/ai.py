from invoke import Context, task
from core import CONFIG, CommandSerializer

@task
def serialize(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Run the AI Content Serializer script."""
    script_path = CONFIG.paths.ai_content_serializer_script
    if not script_path.exists():
        raise FileNotFoundError(f"AI script not found: {script_path}")

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(f'python "{script_path}"', extra=opts)
    serializer.run(c, dry_run=dry_run)