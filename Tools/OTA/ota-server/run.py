import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
sys.path.append(str(PROJECT_ROOT))

from src.core.config import settings


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

    print(f"Starting server using parameters from '{PROJECT_ROOT / 'config.yaml'}'")

    env = os.environ.copy()
    env["PYTHONPATH"] = str(PROJECT_ROOT)

    try:
        subprocess.run([sys.executable, str(main_script)], env=env, check=True)
    except KeyboardInterrupt:
        print("\nServer stopped by user.")
    except subprocess.CalledProcessError as e:
        print(f"Server process terminated with exit code: {e.returncode}")
        sys.exit(e.returncode)


if __name__ == "__main__":
    verify_certificates()
    launch_server()