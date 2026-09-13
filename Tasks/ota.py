"""
@file       ota.py
@brief      OTA server orchestration and high-level release/provisioning lifecycle tasks.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

from invoke import Context, task
from core import CONFIG, CommandSerializer
from embedded_system.Tasks.toolchains.factory import get_toolchain
from embedded_system.Tasks.crypto import provision_nvs, provision_sign, provision_flash


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
    """
    OTA Release Lifecycle Sequence:
    1. Compile application firmware using configured platform toolchain.
    2. Compute SHA-256 digest, sign binary via developer key, and update manifest.json.
    """
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
    """
    Factory Provisioning Lifecycle Sequence:
    1. Compile all binaries (bootloader, partition-table, app) via platform toolchain.
    2. Generate AES-XTS encrypted NVS partition (fctry) containing Root CA and identity.
    3. Burn silicon eFuses (Secure Boot V2, Flash Enc, Anti-Rollback) and flash memory layout.
    """
    selected_target = target or getattr(CONFIG.esp32, "target", "esp32s3")
    toolchain = get_toolchain()

    print(f"\n[*] [FACTORY PROVISION: STEP 1/3] Compiling factory firmware for target '{selected_target}'...")
    toolchain.build(c, target=selected_target, dry_run=dry_run, opts=build_opts)

    print(f"\n[*] [FACTORY PROVISION: STEP 2/3] Generating encrypted factory NVS partition...")
    provision_nvs(c, dry_run=dry_run, config=config)

    print(f"\n[*] [FACTORY PROVISION: STEP 3/3] Burning silicon eFuses and flashing layout (Simulate={simulate})...")
    provision_flash(c, simulate=simulate, dry_run=dry_run, config=config)
    print("\n[SUCCESS] Factory build, NVS encryption, and silicon provisioning completed.\n")