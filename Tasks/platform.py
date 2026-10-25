from functools import lru_cache
from pathlib import Path
from invoke import Context, task
from core import CONFIG
from core.config_resolver import resolve_config
from embedded_system.Tools.TOOLCHAIN.factory import get_toolchain
from embedded_system.Tasks.build_config import generate as generate_build_config


@lru_cache(maxsize=1)
def _get_platform_config():
    """Resolve the Platform SSoT (config_platform.yaml) once per session."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()
    default_config = (
        getattr(CONFIG.paths, "es_config_platform", None)
        or Path(getattr(CONFIG.paths, "configs_dir", workspace_root / "configs"))
        / "config_platform.yaml"
    )
    raw_config = Path(default_config)
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()
    return resolve_config(raw_config, workspace_root=workspace_root)


@lru_cache(maxsize=1)
def _get_toolchain():
    """Resolve the active toolchain dynamically from the Platform SSoT."""
    return get_toolchain(config=_get_platform_config())


@task(
    pre=[generate_build_config],
    help={
        "target": "Target SoC (defaults to config_tasks.yaml target)",
        "image_bin": "Merged binary path (defaults to factory.bin)",
        "dry_run": "Print commands without executing",
        "opts": "Forward extra build options to idf.py",
    },
)
def build(
    c: Context,
    target: str = "",
    image_bin: str = "",
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Build application firmware and generate merged factory binary."""
    _get_toolchain().build(
        c, target=target, image_bin=image_bin, dry_run=dry_run, opts=opts
    )


@task(
    help={
        "port": "Serial communication port (e.g. COM3, /dev/ttyUSB0)",
        "dry_run": "Print commands without executing",
        "opts": "Forward extra flash options to idf.py",
    }
)
def flash(
    c: Context,
    port: str = "",
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Flash firmware binaries to physical hardware via USB/serial."""
    _get_toolchain().flash(c, port=port, dry_run=dry_run, opts=opts)


@task(
    help={
        "port": "Target device serial port or IP address",
        "ota_port": "Local HTTP test server port serving binary",
        "dry_run": "Print commands without executing",
        "opts": "Forward extra options to pytest",
    }
)
def flash_ota(
    c: Context,
    port: str = "",
    ota_port: int = 8032,
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Execute Over-The-Air (OTA) firmware upgrade verification."""
    _get_toolchain().flash_ota(
        c, port=port, ota_port=ota_port, dry_run=dry_run, opts=opts
    )


@task(
    help={
        "port": "Serial communication port",
        "dry_run": "Print commands without executing",
        "opts": "Forward extra options to serial monitor",
    }
)
def monitor(
    c: Context,
    port: str = "",
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Open interactive serial monitor."""
    _get_toolchain().monitor(c, port=port, dry_run=dry_run, opts=opts)


@task(
    help={
        "port": "Serial communication port",
        "dry_run": "Print commands without executing",
        "opts": "Forward extra options to esptool",
    }
)
def erase(
    c: Context,
    port: str = "",
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Erase entire hardware flash memory."""
    _get_toolchain().erase(c, port=port, dry_run=dry_run, opts=opts)


@task(
    help={
        "dry_run": "Print commands without executing",
        "opts": "Forward extra options to menuconfig",
    }
)
def menuconfig(
    c: Context,
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Open terminal-based Kconfig configuration menu."""
    _get_toolchain().configure(c, dry_run=dry_run, opts=opts)


@task(
    help={
        "dry_run": "Print commands without executing",
        "opts": "Forward extra test flags to runner",
    }
)
def test(
    c: Context,
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Execute host-based Linux emulation unit tests."""
    _get_toolchain().test(c, dry_run=dry_run, opts=opts)


@task(help={"dry_run": "Print commands without executing"})
def clean(c: Context, dry_run: bool = False) -> None:
    """Clean build directory artifacts."""
    _get_toolchain().clean(c, dry_run=dry_run)


@task(help={"dry_run": "Print commands without executing"})
def clean_all(c: Context, dry_run: bool = False) -> None:
    """Purge entire build directory and configuration files (fullclean)."""
    _get_toolchain().clean_all(c, dry_run=dry_run)
