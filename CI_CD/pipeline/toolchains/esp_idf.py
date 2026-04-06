# pipeline/toolchains/esp_idf.py
import os
import threading
import http.server
import socketserver
from ..core.utils import run_cmd
from .base import BaseToolchain

class EspIdfToolchain(BaseToolchain):
    def __init__(self, config):
        super().__init__(config)
        
        # 1. Pull the 2-in-1 image containing BOTH ESP-Matter and ESP-IDF 5.4!
        self.docker_image = "espressif/esp-matter:release-v1.4.2_idf_v5.4.1" 

    def _execute(self, cmd_str, workdir=None):
        """Executes a command either in Docker or locally based on the config."""
        
        if workdir is None:
            workdir = self.config.WORKDIR

        if getattr(self.config, 'USE_DOCKER', True):
            docker_workdir = workdir.replace(self.config.PROJECT_ROOT, "/project").replace("\\", "/")
            
            # --- THE FIX ---
            # ESP-Matter's export.sh changes the directory. 
            # We MUST add `cd {docker_workdir}` to force it back to your project!
            matter_cmd = (
                "source /opt/espressif/esp-idf/export.sh && "
                "source /opt/espressif/esp-matter/export.sh && "
                f"cd {docker_workdir} && "
                f"{cmd_str}"
            )
            
            cmd =[
                "docker", "run", "--rm",
                "-v", f"{self.config.PROJECT_ROOT}:/project",
                "-w", docker_workdir,
                self.docker_image,
                "bash", "-c", matter_cmd
            ]
            run_cmd(cmd)
        else:
            print(f"💻 Running locally in {workdir}...")
            cmd =["bash", "-c", cmd_str]
            run_cmd(cmd, cwd=workdir)

    def _build(self, target):
            # 1. Remove stale configs
            # 2. Export target
            # 3. FORCE the absolute path to sdkconfig.defaults so it is found
            #    even if we are deep inside temp_workspace!

            build_cmd = (
                f"rm -rf build sdkconfig && "
                f"export IDF_TARGET={target} && "
                f"export SDKCONFIG_DEFAULTS=/project/sdkconfig.defaults && "
                f"idf.py build"
            )

            # build_cmd = (
                # f"idf.py -DSDKCONFIG_DEFAULTS=sdkconfig.defaults set-target {target} build"
                # f"idf.py build"
            # )
            self._execute(build_cmd, workdir=self.config.WORKDIR)
    
    def clean(self):
        self._execute("idf.py clean", workdir=self.config.WORKDIR)

    def clean_all(self):
        self._execute("idf.py fullclean", workdir=self.config.WORKDIR)

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