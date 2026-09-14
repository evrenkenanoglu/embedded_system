import argparse
import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
sys.path.append(str(PROJECT_ROOT))

# Parse CLI configuration before initializing Settings
parser = argparse.ArgumentParser(description="Secure Embedded OTA Server")
parser.add_argument("--config", "-c", type=Path, default=None, help="Path to config_ota_server.yaml")
args, remaining_args = parser.parse_known_args()

if args.config:
    os.environ["OTA_CONFIG_PATH"] = str(args.config.resolve())

from src.core.config import settings, CONFIG_FILE


def verify_certificates():
    """Verifies that required SSL/TLS credentials exist prior to launching the server."""
    missing = []
    if not settings.SSL_CERT_FILE.exists():
        missing.append(str(settings.SSL_CERT_FILE))
    if not settings.SSL_KEY_FILE.exists():
        missing.append(str(settings.SSL_KEY_FILE))

    if missing:
        print("ERROR: Required SSL/TLS credentials are missing:")
        for item in missing:
            print(f"  - {item}")
        print("\nGenerate the certificates prior to launching the server (e.g. via Tools/PKI/generate_pki.py).")
        sys.exit(1)


def launch_server():
    """Launches the server subprocess with configured environment."""
    main_script = PROJECT_ROOT / "src" / "main.py"

    if not main_script.exists():
        print(f"ERROR: Entry point script not found at '{main_script}'")
        sys.exit(1)

    print(f"Starting server using parameters from '{CONFIG_FILE}'")

    env = os.environ.copy()
    env["PYTHONPATH"] = str(PROJECT_ROOT)
    if args.config:
        env["OTA_CONFIG_PATH"] = str(args.config.resolve())

    cmd = [sys.executable, str(main_script)]
    if args.config:
        cmd.extend(["--config", str(args.config.resolve())])
    cmd.extend(remaining_args)

    try:
        subprocess.run(cmd, env=env, check=True)
    except KeyboardInterrupt:
        print("\nServer stopped by user.")
    except subprocess.CalledProcessError as e:
        print(f"Server process terminated with exit code: {e.returncode}")
        sys.exit(e.returncode)


if __name__ == "__main__":
    verify_certificates()
    launch_server()
