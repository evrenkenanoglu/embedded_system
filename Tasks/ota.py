from invoke import Context, task
from core import CONFIG, CommandSerializer


@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom OTA server configuration file (defaults to CONFIG.paths.es_config_ota_server)",
        "opts": "Forward arbitrary arguments to the OTA server script",
    }
)
def server(c: Context, dry_run: bool = False, config: str = "", opts: str = "") -> None:
    """Start the Embedded System OTA server."""
    script_path = CONFIG.paths.ota_server_script
    if not script_path.exists():
        raise FileNotFoundError(f"OTA server script not found: {script_path}")

    # Resolve configuration file
    resolved_config = config or getattr(CONFIG.paths, "es_config_ota_server", "")
    config_arg = f'--config "{resolved_config}"' if resolved_config and str(resolved_config).strip() else ""

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )

    cmd = f'python "{script_path}"'
    if config_arg:
        cmd = f"{cmd} {config_arg}"

    # Pass the compiled cmd variable containing config_arg
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)