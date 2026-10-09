from pathlib import Path
from typing import Literal
from invoke import Context, task
from core import CONFIG, CommandSerializer
from core.config_resolver import resolve_to_file
import os
import re
import shutil

FormatLang = Literal["all", "c", "python", "yaml", "cmake"]
FormatMode = Literal["apply", "check"]
FormatScope = Literal["changed", "all"]

VALID_FORMAT_LANGS = ("all", "c", "python", "yaml", "cmake")
VALID_FORMAT_MODES = ("apply", "check")
VALID_FORMAT_SCOPES = ("changed", "all")


@task(
    help={
        "dry_run": "Print execution command without running",
        "opts": "Forward arbitrary arguments to the system file generator script",
    }
)
def system_file_generator(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Run the System Source Files Generator script."""
    script_path = CONFIG.paths.system_file_generator_script
    if not script_path.exists():
        raise FileNotFoundError(
            f"System file generator script not found: {script_path}"
        )

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(f'python "{script_path}"', extra=opts)
    serializer.run(c, dry_run=dry_run)


def _format_code(
    c: Context,
    mode: FormatMode = "apply",
    lang: FormatLang = "all",
    scope: FormatScope = "changed",
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Internal helper to resolve SSoT config to a flat temporary file and invoke code formatter."""
    if mode not in VALID_FORMAT_MODES:
        raise ValueError(
            f"Invalid mode '{mode}'. Available options: {', '.join(VALID_FORMAT_MODES)}"
        )

    if lang not in VALID_FORMAT_LANGS:
        raise ValueError(
            f"Invalid language '{lang}'. Available options: {', '.join(VALID_FORMAT_LANGS)}"
        )

    if scope not in VALID_FORMAT_SCOPES:
        raise ValueError(
            f"Invalid scope '{scope}'. Available options: {', '.join(VALID_FORMAT_SCOPES)}"
        )

    script_path = Path(CONFIG.paths.code_formatter_script).resolve()
    if not script_path.exists():
        raise FileNotFoundError(f"Code formatter script not found: {script_path}")

    # 1. Identify input template config
    raw_config = config or getattr(CONFIG.paths, "es_config_formatter", "")
    if not raw_config:
        raise ValueError("No formatter configuration file specified.")

    # 2. Compile into a flat, standalone YAML file in build/configs/
    resolved_config_file = resolve_to_file(
        raw_config, workspace_root=CONFIG.paths.workspace_dir
    )

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )

    cmd = (
        f'python "{script_path}" '
        f'--config "{resolved_config_file}" '
        f"--mode {mode} "
        f"--lang {lang} "
        f"--scope {scope}"
    )
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "lang": f"Target language: {', '.join(VALID_FORMAT_LANGS)} (default: 'all')",
        "scope": f"File scope: {', '.join(VALID_FORMAT_SCOPES)} (default: 'changed')",
        "dry_run": "Print command line without executing",
        "config": "Path to custom formatter configuration file",
        "opts": "Forward additional arbitrary arguments",
    }
)
def format_apply(
    c: Context,
    lang: FormatLang = "all",
    scope: FormatScope = "changed",
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Apply formatting in-place (defaults to only git-modified/uncommitted files)."""
    _format_code(
        c,
        mode="apply",
        lang=lang,
        scope=scope,
        dry_run=dry_run,
        config=config,
        opts=opts,
    )


@task(
    help={
        "lang": f"Target language: {', '.join(VALID_FORMAT_LANGS)} (default: 'all')",
        "scope": f"File scope: {', '.join(VALID_FORMAT_SCOPES)} (default: 'all')",
        "dry_run": "Print command line without executing",
        "config": "Path to custom formatter configuration file",
        "opts": "Forward additional arbitrary arguments",
    }
)
def format_check(
    c: Context,
    lang: FormatLang = "all",
    scope: FormatScope = "all",
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Check formatting across the workspace without modifying files (CI/CD dry-run)."""
    _format_code(
        c,
        mode="check",
        lang=lang,
        scope=scope,
        dry_run=dry_run,
        config=config,
        opts=opts,
    )


def _normalize_wsl_path(path_val: str) -> Path:
    """Translates Windows drive paths (C:\\... or /mnt/c/...) to valid absolute WSL paths."""
    p_str = str(path_val).strip().replace("\\", "/")
    match = re.search(r"([a-zA-Z]):/(.*)", p_str)
    if match:
        drive = match.group(1).lower()
        rest = match.group(2).lstrip("/")
        return Path(f"/mnt/{drive}/{rest}")
    return Path(p_str)


def _to_windows_path(p: Path) -> str:
    """Translates /mnt/c/... mount path back to Windows C:\\... for display."""
    p_str = p.as_posix()
    if p_str.startswith("/mnt/"):
        parts = p_str.split("/")
        drive = parts[2].upper()
        rest = "\\".join(parts[3:])
        return f"{drive}:\\{rest}"
    return str(p)


@task(
    help={
        "dest": "Override catalog destination directory on Windows",
        "certs_dest": "Override certs destination directory on Windows",
        "include_certs": "Also sync SSL certificates",
    }
)
def sync_ota_windows(
    c: Context,
    dest: str = "",
    certs_dest: str = "",
    include_certs: bool = True,
) -> None:
    """Sync release-catalog and certs from WSL to the Windows host paths."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve Source Directories
    source_catalog_dir = Path(
        getattr(
            CONFIG.paths,
            "ota_catalog_dir",
            workspace_root / "embedded_system/Tools/OTA/release-catalog",
        )
    ).resolve()

    certs_src = Path(
        getattr(CONFIG.paths, "certs_dir", workspace_root / "certs")
    ).resolve()

    # 2. Resolve Destination Directories
    raw_dest = dest or getattr(
        CONFIG.paths,
        "ota_storage_windows_dest_dir",
        "/mnt/c/WORKSPACE_PERSONAL/PROJECTS/SMART_PLUGS/SW/embedded_system/Tools/OTA/release-catalog",
    )
    dest_catalog_root = _normalize_wsl_path(raw_dest).resolve()

    raw_certs_dest = certs_dest or getattr(
        CONFIG.paths,
        "ota_certs_windows_dest_dir",
        "/mnt/c/WORKSPACE_PERSONAL/PROJECTS/SMART_PLUGS/SW/certs",
    )
    dest_certs_root = _normalize_wsl_path(raw_certs_dest).resolve()

    if not source_catalog_dir.exists():
        print(f"❌ Source release-catalog not found: {source_catalog_dir}")
        return

    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(" ❯ 📦 SYNCING RELEASE CATALOG TO WINDOWS HOST")
    print(f"   Catalog WSL -> Windows : {source_catalog_dir} -> {dest_catalog_root}")
    print(f"   Certs   WSL -> Windows : {certs_src} -> {dest_certs_root}")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    # 3. Sync release-catalog tree (binaries/, patches/, manifest.json, diff_worker.py)
    dest_catalog_root.mkdir(parents=True, exist_ok=True)
    (dest_catalog_root / "binaries").mkdir(parents=True, exist_ok=True)
    (dest_catalog_root / "patches").mkdir(parents=True, exist_ok=True)

    # Sync manifest and diff_worker script
    for fname in ["manifest.json", "diff_worker.py"]:
        src_f = source_catalog_dir / fname
        if src_f.exists():
            shutil.copy2(src_f, dest_catalog_root / fname)

    # Sync binaries/
    src_bin = source_catalog_dir / "binaries"
    if src_bin.exists():
        for bfile in src_bin.glob("*.bin"):
            shutil.copy2(bfile, dest_catalog_root / "binaries" / bfile.name)

    # Sync patches/
    src_patch = source_catalog_dir / "patches"
    if src_patch.exists():
        for pfile in src_patch.glob("*.bin"):
            shutil.copy2(pfile, dest_catalog_root / "patches" / pfile.name)

    print(f"✅ Synced release-catalog -> {dest_catalog_root}")

    # 4. Sync SSL certificates
    if include_certs and certs_src.exists():
        dest_certs_root.mkdir(parents=True, exist_ok=True)
        for cert_file in certs_src.glob("*.*"):
            if cert_file.is_file():
                shutil.copy2(cert_file, dest_certs_root / cert_file.name)
        print(f"✅ Synced SSL certificates -> {dest_certs_root}")

    win_server_path = _to_windows_path(dest_catalog_root.parent / "ota-server")
    print("\n🎉 Sync completed successfully!")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("💡 To run the server on Windows PowerShell:")
    print(f'   cd "{win_server_path}"')
    print("   python run.py")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
