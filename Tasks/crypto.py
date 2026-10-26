"""
@file       crypto.py
@brief      Invoke tasks for PKI key generation, firmware code-signing, and silicon provisioning.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import sys
from pathlib import Path
from invoke import Context, task
from core import CONFIG
from core.config_resolver import resolve_to_file

VALID_PROVISION_STEPS = ("all", "nvs", "sign", "provision")


@task(
    help={
        "dry_run": "Print execution command without running",
        "config_path": "Path to custom PKI configuration file (overrides SSoT)",
        "opts": "Forward arbitrary arguments to the PKI generation tool",
    }
)
def generate_pki(
    c: Context, dry_run: bool = False, config_path: str = "", opts: str = ""
) -> None:
    """Resolve SSoT config to a flat file and delegate to the standalone PKI tool task."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Identify input template config
    raw_config = (
        Path(config_path)
        if config_path and str(config_path).strip()
        else Path(getattr(CONFIG.paths, "es_config_pki", ""))
    )
    if not str(raw_config).strip() or str(raw_config) == ".":
        raise ValueError("No PKI configuration file specified.")
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    # 2. Compile into a flat, standalone YAML file in build/configs/
    resolved_config_file = resolve_to_file(raw_config, workspace_root=workspace_root)

    # 3. Delegate to the tool's runtime task (zero raw python script paths)
    opts_flag = f'--opts="{opts.strip()}"' if opts and str(opts).strip() else ""
    c.run(
        f'"{sys.executable}" -m invoke tools.pki.generate --config="{resolved_config_file}" {opts_flag}',
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )


@task(
    help={
        "port": "Serial port override",
        "baud": "Baud rate override",
        "config_path": "Path to custom provisioning config file (overrides SSoT)",
        "dry_run": "Print command line without executing",
    }
)
def flash_nvs(
    c: Context,
    port: str = "",
    baud: str = "",
    config_path: str = "",
    dry_run: bool = False,
) -> None:
    """Delegate NVS flashing to the provisioning tool task."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()
    raw_config = (
        Path(config_path)
        if config_path and str(config_path).strip()
        else Path(getattr(CONFIG.paths, "es_config_provisioning", ""))
    )
    if not str(raw_config).strip() or str(raw_config) == ".":
        raise ValueError("No provisioning configuration file specified.")
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config = resolve_to_file(raw_config, workspace_root=workspace_root)
    flags = [f'--config-path="{resolved_config}"']
    if port:
        flags.append(f'--port="{port}"')
    if baud:
        flags.append(f'--baud="{baud}"')

    c.run(
        f'"{sys.executable}" -m invoke tools.provisioning.flash-nvs {" ".join(flags)}',
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )


@task(
    help={
        "step": f"Target provisioning step: {', '.join(VALID_PROVISION_STEPS)} (default: 'all')",
        "target_mode": "Execution target: 'virtual' (default, zero risk) or 'hardware' (physical silicon)",
        "simulate": "Simulate without writing files/flashing (dry-run)",
        "burn_password": "Passphrase required when --target-mode=hardware to burn eFuses",
        "continuous": "Run in automated continuous fixture loop awaiting device insertion",
        "dry_run": "Print command line without executing",
        "config_path": "Path to custom provisioning config file (overrides SSoT)",
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
    config_path: str = "",
    opts: str = "",
) -> None:
    """Resolve SSoT config to a flat file and delegate to the provisioning tool task."""
    if step not in VALID_PROVISION_STEPS:
        raise ValueError(
            f"Invalid step '{step}'. Available options: {', '.join(VALID_PROVISION_STEPS)}"
        )

    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve SSoT config at this level
    raw_config = (
        Path(config_path)
        if config_path and str(config_path).strip()
        else Path(getattr(CONFIG.paths, "es_config_provisioning", ""))
    )
    if not str(raw_config).strip() or str(raw_config) == ".":
        raise ValueError("No provisioning configuration file specified.")
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config_file = resolve_to_file(raw_config, workspace_root=workspace_root)

    # 2. Delegate to the tool's runtime task (zero raw python script paths)
    flags = [
        f'--config="{resolved_config_file}"',
        f"--step {step}",
        f"--target-mode {target_mode}",
    ]
    if simulate:
        flags.append("--simulate")
    if continuous:
        flags.append("--continuous")
    if burn_password:
        flags.append(f'--burn-password "{burn_password}"')

    opts_flag = f' --opts="{opts.strip()}"' if opts and str(opts).strip() else ""
    c.run(
        f'"{sys.executable}" -m invoke tools.provisioning.run {" ".join(flags)}{opts_flag}',
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )


@task(
    help={
        "dry_run": "Print command line without executing",
        "config_path": "Config override",
        "opts": "Additional options",
    }
)
def provision_nvs(
    c: Context, dry_run: bool = False, config_path: str = "", opts: str = ""
) -> None:
    """Step 1: Generate encrypted NVS partition binary and encryption keys."""
    provision_hardware(
        c, step="nvs", dry_run=dry_run, config_path=config_path, opts=opts
    )


@task(
    help={
        "dry_run": "Print command line without running",
        "config_path": "Path to custom provisioning config file (overrides SSoT)",
    }
)
def verify_nvs(c: Context, dry_run: bool = False, config_path: str = "") -> None:
    """Verify generated NVS key and encrypted partition artifacts without device I/O."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()
    raw_config = (
        Path(config_path)
        if config_path and str(config_path).strip()
        else Path(getattr(CONFIG.paths, "es_config_provisioning", ""))
    )
    if not str(raw_config).strip() or str(raw_config) == ".":
        raise ValueError("No provisioning configuration file specified.")
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config = resolve_to_file(raw_config, workspace_root=workspace_root)
    c.run(
        f'"{sys.executable}" -m invoke tools.provisioning.verify-nvs '
        f'--config-path="{resolved_config}"',
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )


@task(
    help={
        "dry_run": "Print command line without executing",
        "config_path": "Config override",
        "opts": "Additional options",
    }
)
def provision_sign(
    c: Context, dry_run: bool = False, config_path: str = "", opts: str = ""
) -> None:
    """Step 2: Sign application binary and package release into server manifest."""
    provision_hardware(
        c, step="sign", dry_run=dry_run, config_path=config_path, opts=opts
    )


@task(
    help={
        "target_mode": "Execution target: 'virtual' (default, zero risk) or 'hardware' (physical silicon)",
        "simulate": "Simulate flashing without burning eFuses or writing flash",
        "burn_password": "Required passphrase to authorize physical eFuse burns",
        "continuous": "Run in automated continuous fixture loop awaiting device insertion",
        "dry_run": "Print command line without executing",
        "config_path": "Config override",
        "opts": "Additional options",
    }
)
def provision_flash(
    c: Context,
    target_mode: str = "virtual",
    simulate: bool = False,
    burn_password: str = "",
    continuous: bool = False,
    dry_run: bool = False,
    config_path: str = "",
    opts: str = "",
) -> None:
    """Step 3: Burn hardware eFuses and flash encrypted image layout to physical ESP32."""
    provision_hardware(
        c,
        step="provision",
        target_mode=target_mode,
        simulate=simulate,
        burn_password=burn_password,
        continuous=continuous,
        dry_run=dry_run,
        config_path=config_path,
        opts=opts,
    )
