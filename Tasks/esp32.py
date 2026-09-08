from invoke import Context, task
from core import CONFIG, CommandSerializer

def _get_idf_serializer() -> CommandSerializer:
    """Helper to activate ESP-IDF environment."""
    return CommandSerializer(
        prefix_commands=["source activate_esp-5.4-matter_1.4.2.sh"],
        env=CONFIG.env,
    )

@task(
    help={
        "dry_run": "Print command without running",
        "opts": "Forward extra flags to idf.py build",
    }
)
def build(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Build the project from workspace root using idf.py build."""
    work_dir = CONFIG.paths.workspace_dir
    cmd = f'cd "{work_dir}" && idf.py build'

    serializer = _get_idf_serializer()
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)

@task(
    help={
        "dry_run": "Print command without running",
        "opts": "Forward extra flags to idf.py flash",
    }
)
def flash(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Flash the firmware from workspace root using idf.py flash."""
    work_dir = CONFIG.paths.workspace_dir
    cmd = f'cd "{work_dir}" && idf.py flash'

    serializer = _get_idf_serializer()
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)

@task(
    help={
        "dry_run": "Print command without running",
        "opts": "Forward extra flags to idf.py monitor",
    }
)
def monitor(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Open the serial monitor from workspace root using idf.py monitor."""
    work_dir = CONFIG.paths.workspace_dir
    cmd = f'cd "{work_dir}" && idf.py monitor'

    serializer = _get_idf_serializer()
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)

@task(
    help={
        "dry_run": "Print command without running",
    }
)
def clean(c: Context, dry_run: bool = False) -> None:
    """Clean the build directory from workspace root using idf.py fullclean."""
    work_dir = CONFIG.paths.workspace_dir
    cmd = f'cd "{work_dir}" && idf.py fullclean'

    serializer = _get_idf_serializer()
    serializer.add(cmd)
    serializer.run(c, dry_run=dry_run)

@task(
    help={
        "dry_run": "Print command without running",
    }
)
def erase(c: Context, dry_run: bool = False) -> None:
    """Erase flash memory from workspace root using idf.py erase-flash."""
    work_dir = CONFIG.paths.workspace_dir
    cmd = f'cd "{work_dir}" && idf.py erase-flash'

    serializer = _get_idf_serializer()
    serializer.add(cmd)
    serializer.run(c, dry_run=dry_run)

@task(
    help={
        "dry_run": "Print command without running",
    }
)
def menuconfig(c: Context, dry_run: bool = False) -> None:
    """Open project configuration from workspace root using idf.py menuconfig."""
    work_dir = CONFIG.paths.workspace_dir
    cmd = f'cd "{work_dir}" && idf.py menuconfig'

    serializer = _get_idf_serializer()
    serializer.add(cmd)
    serializer.run(c, dry_run=dry_run)