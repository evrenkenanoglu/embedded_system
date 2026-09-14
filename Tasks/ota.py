"""
@file       ota.py
@brief      OTA server orchestration and high-level release/provisioning lifecycle tasks.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer
from core.config_resolver import resolve_to_file
from embedded_system.Tasks.toolchains.factory import get_toolchain
from embedded_system.Tasks.crypto import provision_nvs, provision_sign, provision_flash


@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom OTA server configuration file (defaults to configs/config_ota_server.yaml)",
        "opts": "Forward arbitrary arguments to the OTA server script",
    }
)
def server(c: Context, dry_run: bool = False, config: str = "", opts: str = "") -> None:
    """Start the Embedded System OTA server with compiled SSoT configuration."""
    script_path = Path(CONFIG.paths.ota_server_script).resolve()
    if not script_path.exists():
        raise FileNotFoundError(f"OTA server script not found: {script_path}")

    # 1. Resolve raw input config path
    raw_config = Path(config) if config and str(config).strip() else Path(CONFIG.paths.es_config_ota_server)
    if not raw_config.is_absolute():
        raw_config = (Path(CONFIG.paths.workspace_dir) / raw_config).resolve()

    # 2. Compile into flat, standalone YAML in build/configs/
    resolved_config_file = resolve_to_file(raw_config, workspace_root=CONFIG.paths.workspace_dir)

    # 3. Pass via CLI flag AND OTA_CONFIG_PATH env var for Uvicorn reload workers
    execution_env = dict(CONFIG.env or {})
    execution_env["OTA_CONFIG_PATH"] = str(resolved_config_file)

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=execution_env,
    )

    cmd = f'python "{script_path}" --config "{resolved_config_file}"'
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "target": "Target SoC architecture (defaults to CONFIG.esp32.target or 'esp32s3')",
        "dry_run": "Print commands without executing",
        "config": "Path to custom provisioning configuration file",
        "build_opts": "Extra flags forwarded to the toolchain build command",
    }
)
def release(
    c: Context,
    target: str = "",
    dry_run: bool = False,
    config: str = "",
    build_opts: str = "",
) -> None:
    """OTA Release Lifecycle Sequence."""
    selected_target = target or getattr(CONFIG.esp32, "target", "esp32s3")
    toolchain = get_toolchain()

    print(f"\n[*] [OTA RELEASE: STEP 1/2] Compiling firmware for target '{selected_target}'...")
    toolchain.build(c, target=selected_target, dry_run=dry_run, opts=build_opts)

    print(f"\n[*] [OTA RELEASE: STEP 2/2] Signing release binary and packaging manifest...")
    provision_sign(c, dry_run=dry_run, config=config)
    print("\n[SUCCESS] OTA release compilation, code-signing, and catalog update completed.\n")


@task(
    help={
        "target": "Target SoC architecture (defaults to CONFIG.esp32.target or 'esp32s3')",
        "simulate": "Simulate flashing without burning eFuses or writing physical flash",
        "dry_run": "Print commands without executing",
        "config": "Path to custom provisioning configuration file",
        "build_opts": "Extra flags forwarded to the toolchain build command",
    }
)
def factory_provision(
    c: Context,
    target: str = "",
    simulate: bool = True,
    dry_run: bool = False,
    config: str = "",
    build_opts: str = "",
) -> None:
    """Factory Provisioning Lifecycle Sequence."""
    selected_target = target or getattr(CONFIG.esp32, "target", "esp32s3")
    toolchain = get_toolchain()

    print(f"\n[*] [FACTORY PROVISION: STEP 1/3] Compiling factory firmware for target '{selected_target}'...")
    toolchain.build(c, target=selected_target, dry_run=dry_run, opts=build_opts)

    print(f"\n[*] [FACTORY PROVISION: STEP 2/3] Generating encrypted factory NVS partition...")
    provision_nvs(c, dry_run=dry_run, config=config)

    print(f"\n[*] [FACTORY PROVISION: STEP 3/3] Burning silicon eFuses and flashing layout (Simulate={simulate})...")
    provision_flash(c, simulate=simulate, dry_run=dry_run, config=config)
    print("\n[SUCCESS] Factory build, NVS encryption, and silicon provisioning completed.\n")