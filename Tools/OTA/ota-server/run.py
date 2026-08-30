import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
sys.path.append(str(PROJECT_ROOT))

from src.core.config import settings


def check_and_generate_certs():
    """Checks for existing SSL/TLS credentials and generates them if missing."""
    if not settings.SSL_CERT_FILE.exists() or not settings.SSL_KEY_FILE.exists():
        print("SSL credentials missing. Executing generate_certs.py...")
        try:
            from generate_certs import generate_certificates
            generate_certificates()
        except ImportError:
            print("ERROR: Could not import 'generate_certs.py'.")
            sys.exit(1)
        except Exception as e:
            print(f"ERROR: Certificate generation failed: {e}")
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
    check_and_generate_certs()
    launch_server()