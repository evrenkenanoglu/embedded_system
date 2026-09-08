from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer


@task
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


@task
def provision_hardware(c: Context, dry_run: bool = False, config: str = "", opts: str = "") -> None:
    """Run the Provision Hardware script."""
    script_path = CONFIG.paths.provision_hardware_script
    if not script_path.exists():
        raise FileNotFoundError(f"Provision hardware script not found: {script_path}")

    # Resolve configuration file: explicit task argument takes priority over CONFIG.paths
    resolved_config = config or getattr(CONFIG.paths, "es_config_provisioning", "")
    config_arg = f'--config "{resolved_config}"' if resolved_config and str(resolved_config).strip() else ""

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )

    cmd = f'python "{script_path}"'
    if config_arg:
        cmd = f"{cmd} {config_arg}"

    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)


@task
def generate_pki(c: Context, dry_run: bool = False, config: str = "", opts: str = "") -> None:
    """Run the PKI generation script."""
    script_path = CONFIG.paths.generate_pki_script
    if not script_path.exists():
        raise FileNotFoundError(f"PKI generation script not found: {script_path}")

    # Resolve configuration file: explicit task argument takes priority over CONFIG.paths
    resolved_config = config or getattr(CONFIG.paths, "es_config_pki", "")
    config_arg = f'--config "{resolved_config}"' if resolved_config and str(resolved_config).strip() else ""

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )

    cmd = f'python "{script_path}"'
    if config_arg:
        cmd = f"{cmd} {config_arg}"

    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)