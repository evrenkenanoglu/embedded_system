from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer

VALID_PROVISION_STEPS = ("all", "nvs", "sign", "provision")

@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom PKI configuration file (defaults to CONFIG.paths.es_config_pki)",
        "opts": "Forward arbitrary arguments to the PKI generation script",
    }
)
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

@task(
    help={
        "step": f"Target provisioning step to execute: {', '.join(VALID_PROVISION_STEPS)} (default: 'all')",
        "simulate": "Pass --dry-run to main.py (simulates execution without burning physical eFuses or flashing)",
        "dry_run": "Print the command line without executing (Invoke dry run)",
        "config": "Path to custom configuration file (defaults to CONFIG.paths.es_config_provisioning)",
        "opts": "Forward additional arbitrary arguments to the provisioning script",
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
    """Run the ESP32 Provisioning and Release Orchestrator with step selection."""
    if step not in VALID_PROVISION_STEPS:
        raise ValueError(f"Invalid step '{step}'. Available options: {', '.join(VALID_PROVISION_STEPS)}")

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

    cmd = f'python "{script_path}" --step {step}'
    if config_arg:
        cmd = f"{cmd} {config_arg}"
    if simulate:
        cmd = f"{cmd} --dry-run"

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

