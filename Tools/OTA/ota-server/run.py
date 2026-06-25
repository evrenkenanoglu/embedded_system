import os
import sys
import subprocess
from pathlib import Path

# Resolve absolute directories
PROJECT_ROOT = Path(__file__).resolve().parent
sys.path.append(str(PROJECT_ROOT))

# Paths to verify
CERT_DIR = PROJECT_ROOT / "certificates"
SSL_CERT_FILE = CERT_DIR / "server.crt"
SSL_KEY_FILE = CERT_DIR / "server.key"


def check_and_generate_certs():
    """Checks for existing SSL/TLS credentials and generates them if missing."""
    if not SSL_CERT_FILE.exists() or not SSL_KEY_FILE.exists():
        print("SSL credentials not found. Initiating local certificate generation...")
        try:
            # Import and execute the certificate generator inline
            from generate_certs import generate_certificates
            generate_certificates()
        except ImportError:
            print("ERROR: Could not import 'generate_certs.py'. Ensure the file is in the project root.")
            sys.exit(1)
        except Exception as e:
            print(f"ERROR: Certificate generation failed: {e}")
            sys.exit(1)


def launch_server():
    """Launches the main server application as a subprocess with PYTHONPATH configured."""
    main_script = PROJECT_ROOT / "src" / "main.py"
    
    if not main_script.exists():
        print(f"ERROR: Entry point script not found at {main_script}")
        sys.exit(1)

    print("Launching Local Secure OTA Server...")
    
    # Configure environment to include the project root in the Python module search path
    env = os.environ.copy()
    env["PYTHONPATH"] = str(PROJECT_ROOT)

    try:
        # Launch main.py on any host operating system
        subprocess.run([sys.executable, str(main_script)], env=env, check=True)
    except KeyboardInterrupt:
        print("\nServer stopped by user.")
    except subprocess.CalledProcessError as e:
        print(f"Server exited with error code: {e.returncode}")
        sys.exit(e.returncode)


if __name__ == "__main__":
    check_and_generate_certs()
    launch_server()