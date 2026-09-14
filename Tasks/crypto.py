from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer
from core.config_resolver import resolve_to_file

VALID_PROVISION_STEPS = ("all", "nvs", "sign", "provision")


@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom PKI configuration file (defaults to CONFIG.paths.es_config_pki)",
        "opts": "Forward arbitrary arguments to the PKI generation script",
    }
)
def generate_pki(c: Context, dry_run: bool = False, config: str = "", opts: str = "") -> None:
    """Resolve SSoT config to a flat temporary file and invoke standalone PKI generator."""
    script_path = Path(CONFIG.paths.generate_pki_script).resolve()
    if not script_path.exists():
        raise FileNotFoundError(f"PKI generation script not found: {script_path}")

    # 1. Identify input template config
    raw_config = config or getattr(CONFIG.paths, "es_config_pki", "")
    if not raw_config:
        raise ValueError("No PKI configuration file specified.")

    # 2. Compile into a flat, standalone YAML file in build/configs/
    resolved_config_file = resolve_to_file(raw_config, workspace_root=CONFIG.paths.workspace_dir)

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )

    cmd = f'python "{script_path}" --config "{resolved_config_file}"'
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "step": f"Target provisioning step to execute: {', '.join(VALID_PROVISION_STEPS)} (default: 'all')",
        "simulate": "Pass --dry-run to main.py",
        "dry_run": "Print command line without executing",
        "config": "Path to custom provisioning config file",
        "opts": "Forward additional arbitrary arguments",
    }
)
def provision_hardware(
    c: Context,
    step: str = "all",
    simulate: bool = False,
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Resolve SSoT config to a flat temporary file and invoke standalone provisioning script."""
    if step not in VALID_PROVISION_STEPS:
        raise ValueError(f"Invalid step '{step}'. Available options: {', '.join(VALID_PROVISION_STEPS)}")

    script_path = Path(CONFIG.paths.provision_hardware_script).resolve()
    if not script_path.exists():
        raise FileNotFoundError(f"Provision hardware script not found: {script_path}")

    raw_config = config or getattr(CONFIG.paths, "es_config_provisioning", "")
    if not raw_config:
        raise ValueError("No provisioning configuration file specified.")

    resolved_config_file = resolve_to_file(raw_config, workspace_root=CONFIG.paths.workspace_dir)

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )

    cmd = f'python "{script_path}" --step {step} --config "{resolved_config_file}"'
    if simulate:
        cmd += " --dry-run"

    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "dry_run": "Print command line without executing",
        "config": "Config override",
        "opts": "Additional options",
    }
)
def provision_nvs(c: Context, dry_run: bool = False, config: str = "", opts: str = "") -> None:
    """Step 1: Generate encrypted NVS partition binary and encryption keys."""
    provision_hardware(c, step="nvs", dry_run=dry_run, config=config, opts=opts)


@task(
    help={
        "dry_run": "Print command line without executing",
        "config": "Config override",
        "opts": "Additional options",
    }
)
def provision_sign(c: Context, dry_run: bool = False, config: str = "", opts: str = "") -> None:
    """Step 2: Sign application binary and package release into server manifest."""
    provision_hardware(c, step="sign", dry_run=dry_run, config=config, opts=opts)


@task(
    help={
        "simulate": "Simulate flashing without burning eFuses or writing flash",
        "dry_run": "Print command line without executing",
        "config": "Config override",
        "opts": "Additional options",
    }
)
def provision_flash(
    c: Context,
    simulate: bool = False,
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Step 3: Burn hardware eFuses and flash encrypted image layout to physical ESP32."""
    provision_hardware(c, step="provision", simulate=simulate, dry_run=dry_run, config=config, opts=opts)