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


def _get_target_chip(explicit_target: str = "") -> str:
    """Resolves active target silicon dynamically from Master SSoT."""
    if explicit_target:
        return explicit_target
    target_cfg = getattr(CONFIG, "target", None)
    if target_cfg and hasattr(target_cfg, "chip"):
        return target_cfg.chip
    hw_cfg = getattr(CONFIG, "hardware", None)
    if hw_cfg and hasattr(hw_cfg, "chip"):
        return hw_cfg.chip
    esp32_cfg = getattr(CONFIG, "esp32", None)
    if esp32_cfg and hasattr(esp32_cfg, "target"):
        return esp32_cfg.target
    return "esp32c6"


@task(
    help={
        "dry_run": "Print execution command without running",
        "config": "Path to custom OTA server configuration file",
        "opts": "Forward arbitrary arguments to the OTA server script",
    }
)
def server(c: Context, dry_run: bool = False, config: str = "", opts: str = "") -> None:
    """Start the Embedded System OTA server with compiled SSoT configuration."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    default_script = workspace_root / "embedded_system/Tools/OTA/ota-server/run.py"
    raw_script = getattr(CONFIG.paths, "ota_server_script", default_script)
    script_path = Path(raw_script).resolve()
    if not script_path.exists():
        raise FileNotFoundError(f"OTA server script not found: {script_path}")

    default_config = (
        getattr(CONFIG.paths, "es_config_ota_server", None)
        or getattr(CONFIG.paths, "config_ota_server", None)
        or Path(getattr(CONFIG.paths, "configs_dir", workspace_root / "configs")) / "config_ota_server.yaml"
    )
    raw_config = Path(config) if config and str(config).strip() else Path(default_config)
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config_file = resolve_to_file(raw_config, workspace_root=workspace_root)

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
        "target": "Target SoC architecture (defaults to SSoT target.chip)",
        "channel": "Deployment cohort channel (default: stable)",
        "notes": "Changelog entry or release notes",
        "dry_run": "Print commands without executing",
        "sync_windows": "Automatically sync release catalog to Windows host if configured",
        "build_opts": "Extra flags forwarded to compiler",
    }
)
def release(
    c: Context,
    target: str = "",
    channel: str = "stable",
    notes: str = "Production release build",
    dry_run: bool = False,
    sync_windows: bool = True,
    build_opts: str = "",
) -> None:
    """Canonical OTA Release Lifecycle: Build -> Sign -> 1-Hop Diff -> Sync."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()
    selected_target = _get_target_chip(target)
    toolchain = get_toolchain()

    # Step 1: Compile firmware
    print(f"\n[*] [OTA RELEASE: 1/4] Compiling firmware for target '{selected_target}'...")
    toolchain.build(c, target=selected_target, dry_run=dry_run, opts=build_opts)

    # Step 2: Sign binary and package into release-catalog
    print(f"\n[*] [OTA RELEASE: 2/4] Signing binary and packaging catalog entry...")
    provision_sign(c, dry_run=dry_run)

    # Step 3: Run standalone diff_worker.py (Zero web server imports!)
    catalog_dir = Path(
        getattr(CONFIG.paths, "ota_catalog_dir", workspace_root / "embedded_system/Tools/OTA/release-catalog")
    ).resolve()
    diff_worker_script = catalog_dir / "diff_worker.py"

    if diff_worker_script.exists():
        print(f"\n[*] [OTA RELEASE: 3/4] Reconciling 1-hop delta patch via diff_worker.py...")
        serializer = CommandSerializer(
            prefix_commands=[CONFIG.venv_activate_cmd],
            env=CONFIG.env,
        )
        serializer.add(f'python "{diff_worker_script}" --catalog-dir "{catalog_dir}" --reconcile')
        serializer.run(c, dry_run=dry_run)
    else:
        print(f"⚠️ [WARN] diff_worker.py not found at: {diff_worker_script}. Skipping delta generation.")

    # Step 4: Sync release-catalog to Windows host
    if sync_windows:
        print(f"\n[*] [OTA RELEASE: 4/4] Syncing release catalog to Windows host...")
        from embedded_system.Tasks.util import sync_ota_windows
        sync_ota_windows(c)

    print("\n[SUCCESS] OTA release compilation, code-signing, delta generation, and sync complete!\n")


@task(
    help={
        "target": "Target SoC architecture (defaults to SSoT target.chip)",
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
    selected_target = _get_target_chip(target)
    toolchain = get_toolchain()

    print(f"\n[*] [FACTORY PROVISION: STEP 1/3] Compiling factory firmware for target '{selected_target}'...")
    toolchain.build(c, target=selected_target, dry_run=dry_run, opts=build_opts)

    print(f"\n[*] [FACTORY PROVISION: STEP 2/3] Generating encrypted factory NVS partition...")
    provision_nvs(c, dry_run=dry_run, config=config)

    print(f"\n[*] [FACTORY PROVISION: STEP 3/3] Burning silicon eFuses and flashing layout (Simulate={simulate})...")
    provision_flash(c, simulate=simulate, dry_run=dry_run, config=config)
    print("\n[SUCCESS] Factory build, NVS encryption, and silicon provisioning completed.\n")


@task(
    help={
        "url": "Destination upload URL endpoint (defaults to network.gateway_url/upload)",
        "insecure": "Bypass TLS certificate verification (default: True for local dev IP)",
        "notes": "Custom release notes or changelog entry",
        "channel": "Deployment channel (default: stable)",
        "dry_run": "Print upload command without executing",
    }
)
def upload(
    c: Context,
    url: str = "",
    insecure: bool = True,
    notes: str = "Automated CLI build upload",
    channel: str = "stable",
    dry_run: bool = False,
) -> None:
    """Upload compiled firmware to an external/remote OTA server via HTTP API."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()
    script_path = (workspace_root / "embedded_system/Tools/OTA/ota-server/upload_firmware.py").resolve()

    default_config = (
        getattr(CONFIG.paths, "config_project", None)
        or Path(getattr(CONFIG.paths, "configs_dir", workspace_root / "configs")) / "config_project.yaml"
    )
    raw_config = Path(default_config)
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config = resolve_to_file(raw_config, workspace_root=workspace_root)

    cmd_parts = [
        f'python "{script_path}"',
        f'--config "{resolved_config}"',
        f'--channel "{channel}"',
        f'--notes "{notes}"',
    ]

    if url and str(url).strip():
        cmd_parts.append(f'--url "{url}"')
    if insecure:
        cmd_parts.append("--insecure")

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(" ".join(cmd_parts))
    serializer.run(c, dry_run=dry_run)
