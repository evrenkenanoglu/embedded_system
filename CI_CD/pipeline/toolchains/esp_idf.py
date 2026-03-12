import os
import threading
import http.server
import socketserver
from ..core.utils import run_cmd
from .base import BaseToolchain

class EspIdfToolchain(BaseToolchain):
    def __init__(self, config):
        super().__init__(config)
        self.docker_image = f"espressif/idf:release-v{self.config.IDF_VERSION}"

    def _run_in_docker(self, cmd_str, workdir="/project"):
        cmd =[
            "docker", "run", "--rm",
            "-v", f"{self.config.PROJECT_ROOT}:/project",
            "-w", workdir,
            self.docker_image,
            "bash", "-c", cmd_str
        ]
        run_cmd(cmd)

    def _build(self, target):
        self._run_in_docker(f"idf.py set-target {target} && idf.py build")

    def _run_unit_tests(self):
        self._run_in_docker("idf.py set-target linux && idf.py build && ./build/unit_test_app", workdir="/project/tests/unit")

    def _flash_usb(self, port, binary_dir):
        args_file = os.path.join(binary_dir, self.config.FLASH_ARGS_FILE)
        run_cmd(["python", "-m", "esptool", "-p", port, "write_flash", "@" + args_file])

    def _flash_ota(self, port, binary_dir, ota_port):
        class Handler(http.server.SimpleHTTPRequestHandler):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, directory=binary_dir, **kwargs)

        httpd = socketserver.TCPServer(("", ota_port), Handler)
        thread = threading.Thread(target=httpd.serve_forever, daemon=True)
        thread.start()

        run_cmd(["pytest", os.path.join(self.config.HIL_TEST_DIR, "test_ota_trigger.py"), 
                 f"--port={port}", f"--ota-port={ota_port}"])
        httpd.shutdown()