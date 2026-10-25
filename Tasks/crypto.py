"""
@file       crypto.py
@brief      Invoke tasks for PKI key generation, firmware code-signing, and silicon provisioning.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import os
import sys
from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer
from core.config_resolver import resolve_to_file

import importlib.util

VALID_PROVISION_STEPS = ("all", "nvs", "sign", "provision")


def _get_partition_parser_cls():
    parser_path = Path(CONFIG.paths.partition_parser_script).resolve()
    spec = importlib.util.spec_from_file_location("partition_parser", parser_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.PartitionTableParser


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
        f'inv tools.pki.generate --config="{resolved_config_file}" {opts_flag}',
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )


@task(
    help={
        "port": "Serial port override",
        "baud": "Baud rate override",
        "dry_run": "Print command line without executing",
    }
)
def flash_nvs(
    c: Context,
    port: str = "",
    baud: str = "",
    dry_run: bool = False,
) -> None:
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()
    build_dir = Path(CONFIG.paths.build_dir).resolve()
    prov_dir = build_dir / "provisioning"
    partitions_csv = workspace_root / "partitions.csv"

    # Select pre-encrypted or plaintext key binary based on SSoT partition table flags
    ParserCls = _get_partition_parser_cls()
    pt_parser = ParserCls(
        partitions_csv_path=partitions_csv,
        partition_table_offset=int(str(CONFIG.flash_layout.partition_table_offset), 0),
        bootloader_offset=int(str(CONFIG.flash_layout.bootloader_offset), 0),
        app_alignment=int(str(CONFIG.flash_layout.app_alignment), 0),
        data_alignment=int(str(CONFIG.flash_layout.data_alignment), 0),
        partition_table_size=int(str(CONFIG.flash_layout.partition_table_size), 0),
    )

    nvs_keys_entry = pt_parser.get_partition("nvs_keys")
    if nvs_keys_entry.is_encrypted:
        nvs_keys_bin = prov_dir / "nvs_keys_encrypted.bin"
    else:
        nvs_keys_bin = prov_dir / "nvs_keys.bin"

    fctry_bin = prov_dir / "nvs_encrypted.bin"

    if not nvs_keys_bin.exists() or not fctry_bin.exists():
        print(f"❌ Provisioned binaries not found in {prov_dir}.")
        print("   Run 'inv es.crypto.provision-nvs' first.")
        return

    nvs_keys_offset = hex(pt_parser.get_offset("nvs_keys"))
    fctry_offset = hex(pt_parser.get_offset("fctry"))

    target_port = port or getattr(CONFIG.hardware, "default_port", "/dev/ttyACM0")
    target_baud = baud or str(getattr(CONFIG.hardware, "flash_baud", 460800))
    target_chip = str(getattr(CONFIG.target, "chip", "esp32c6")).lower()

    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(f" ❯ ⚡ FLASHING FACTORY NVS PARTITIONS ({target_port})")
    print(f"   nvs_keys ({nvs_keys_offset}) : {nvs_keys_bin.name}")
    print(f"   fctry    ({fctry_offset}) : {fctry_bin.name}")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=_get_execution_env(),
    )

    cmd = (
        f"python -m esptool -p {target_port} -b {target_baud} --chip {target_chip} write_flash "
        f'{nvs_keys_offset} "{nvs_keys_bin}" '
        f'{fctry_offset} "{fctry_bin}"'
    )

    serializer.add(cmd)
    serializer.run(c, dry_run=dry_run)
    print("\n✅ Factory NVS partitions flashed successfully!")


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
