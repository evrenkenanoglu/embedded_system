"""
@file       crypto.py
@brief      Invoke tasks for PKI key generation, firmware code-signing, and silicon provisioning.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import os
from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer
from core.config_resolver import resolve_to_file

VALID_PROVISION_STEPS = ("all", "nvs", "sign", "provision")


def _get_execution_env() -> dict:
    """Constructs environment dictionary with workspace, task-runner, and embedded_system on PYTHONPATH."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()
    task_runner_dir = workspace_root / "Tools" / "task-runner"
    embedded_sys_dir = Path(CONFIG.paths.embedded_system_dir).resolve()

    execution_env = dict(CONFIG.env or {})
    env_paths = [str(workspace_root), str(task_runner_dir), str(embedded_sys_dir)]

    existing_pythonpath = execution_env.get("PYTHONPATH") or os.environ.get(
        "PYTHONPATH", ""
    )
    if existing_pythonpath:
        env_paths.append(existing_pythonpath)

    execution_env["PYTHONPATH"] = os.pathsep.join(env_paths)
    return execution_env


@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom PKI configuration file (defaults to CONFIG.paths.es_config_pki)",
        "opts": "Forward arbitrary arguments to the PKI generation script",
    }
)
def generate_pki(
    c: Context, dry_run: bool = False, config: str = "", opts: str = ""
) -> None:
    """Resolve SSoT config to a flat temporary file and invoke standalone PKI generator."""
    script_path = Path(CONFIG.paths.generate_pki_script).resolve()
    if not script_path.exists():
        raise FileNotFoundError(f"PKI generation script not found: {script_path}")

    # 1. Identify input template config
    raw_config = config or getattr(CONFIG.paths, "es_config_pki", "")
    if not raw_config:
        raise ValueError("No PKI configuration file specified.")

    # 2. Compile into a flat, standalone YAML file in build/configs/
    resolved_config_file = resolve_to_file(
        raw_config, workspace_root=CONFIG.paths.workspace_dir
    )

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=_get_execution_env(),
    )

    cmd = f'python "{script_path}" --config "{resolved_config_file}"'
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "step": f"Target provisioning step: {', '.join(VALID_PROVISION_STEPS)} (default: 'all')",
        "target_mode": "Execution target: 'virtual' (default, zero risk) or 'hardware' (physical silicon)",
        "simulate": "Simulate without writing files/flashing (dry-run)",
        "burn_password": "Passphrase required when --target-mode=hardware to burn eFuses",
        "continuous": "Run in automated continuous fixture loop awaiting device insertion",
        "dry_run": "Print command line without executing",
        "config": "Path to custom provisioning config file",
        "opts": "Forward additional arbitrary arguments",
    }
)
def provision_hardware(
    c: Context,
    step: str = "all",
    target_mode: str = "virtual",
    simulate: bool = False,
    burn_password: str = "",
    continuous: bool = False,
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Resolve SSoT config to a flat temporary file and invoke standalone provisioning script."""
    if step not in VALID_PROVISION_STEPS:
        raise ValueError(
            f"Invalid step '{step}'. Available options: {', '.join(VALID_PROVISION_STEPS)}"
        )

    script_path = Path(CONFIG.paths.provision_hardware_script).resolve()
    if not script_path.exists():
        raise FileNotFoundError(f"Provision hardware script not found: {script_path}")

    raw_config = config or getattr(CONFIG.paths, "es_config_provisioning", "")
    if not raw_config:
        raise ValueError("No provisioning configuration file specified.")

    resolved_config_file = resolve_to_file(
        raw_config, workspace_root=CONFIG.paths.workspace_dir
    )

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=_get_execution_env(),
    )

    cmd = (
        f'python "{script_path}" '
        f"--step {step} "
        f"--target-mode {target_mode} "
        f'--config "{resolved_config_file}"'
    )
    if simulate:
        cmd += " --dry-run"
    if continuous:
        cmd += " --continuous"
    if burn_password:
        cmd += f' --burn-password "{burn_password}"'

    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "dry_run": "Print command line without executing",
        "config": "Config override",
        "opts": "Additional options",
    }
)
def provision_nvs(
    c: Context, dry_run: bool = False, config: str = "", opts: str = ""
) -> None:
    """Step 1: Generate encrypted NVS partition binary and encryption keys."""
    provision_hardware(c, step="nvs", dry_run=dry_run, config=config, opts=opts)


@task(
    help={
        "dry_run": "Print command line without executing",
        "config": "Config override",
        "opts": "Additional options",
    }
)
def provision_sign(
    c: Context, dry_run: bool = False, config: str = "", opts: str = ""
) -> None:
    """Step 2: Sign application binary and package release into server manifest."""
    provision_hardware(c, step="sign", dry_run=dry_run, config=config, opts=opts)


@task(
    help={
        "virtual": "Execute against software-simulated silicon model (zero hardware risk)",
        "simulate": "Simulate flashing without burning eFuses or writing flash",
        "burn_password": "Required passphrase to authorize physical eFuse burns",
        "continuous": "Run in automated continuous fixture loop awaiting device insertion",
        "dry_run": "Print command line without executing",
        "config": "Config override",
        "opts": "Additional options",
    }
)
def provision_flash(
    c: Context,
    virtual: bool = False,
    simulate: bool = False,
    burn_password: str = "",
    continuous: bool = False,
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Step 3: Burn hardware eFuses and flash encrypted image layout to physical ESP32."""
    provision_hardware(
        c,
        step="provision",
        virtual=virtual,
        simulate=simulate,
        burn_password=burn_password,
        continuous=continuous,
        dry_run=dry_run,
        config=config,
        opts=opts,
    )
