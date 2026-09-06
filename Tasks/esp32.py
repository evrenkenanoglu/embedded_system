from invoke import Context, task
from core import CONFIG, CommandSerializer


@task(
    help={
        "port": "Serial port (e.g. COM3, /dev/ttyUSB0)",
        "baud": "Serial connection baud rate",
    }
)
def chip_info(
    c: Context,
    port: str = CONFIG.esp32.port,
    baud: int = CONFIG.esp32.baudrate,
) -> None:
    """Read ESP32 chip information."""
    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(
        f"esptool.py --chip {CONFIG.esp32.chip} --port {port} --baud {baud} chip_id"
    )
    serializer.run(c)


@task(
    help={
        "port": "Serial port",
        "baud": "Flashing baud rate",
        "bin_path": "Path to binary file",
        "dry_run": "Print commands without executing",
        "verify": "Verify flash contents after writing",
        "compress": "Compress data in transfer",
    }
)
def flash_pipeline(
    c: Context,
    port: str = CONFIG.esp32.port,
    baud: int = CONFIG.esp32.flash_baudrate,
    bin_path: str = str(CONFIG.esp32.firmware_bin),
    dry_run: bool = False,
    verify: bool = False,
    compress: bool = False,
) -> None:
    """Execute erase and flashing sequence."""
    flags = []
    if verify:
        flags.append("--verify")
    if compress:
        flags.append("--compress")

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(f"esptool.py --chip {CONFIG.esp32.chip} --port {port} erase_flash")
    serializer.add(
        f"esptool.py --chip {CONFIG.esp32.chip} --port {port} --baud {baud} "
        f"write_flash -z {CONFIG.esp32.flash_offset} {Path(bin_path)}",
        extra=" ".join(flags),
    )
    serializer.run(c, dry_run=dry_run)


def build(
    c: Context, target: str = "esp32", image_bin: str = "build/factory.bin"
) -> None:
    """Build the ESP32 project using idf.py."""
    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(
        f"rm -rf build sdkconfig && "
        f"export IDF_TARGET={target} && "
        f"export SDKCONFIG_DEFAULTS=/project/sdkconfig.defaults && "
        f"idf.py build && "
        f"cd build && esptool.py --chip {target} merge_bin -o {image_bin} @flash_args"
    )
    serializer.run(c)
