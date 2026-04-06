# pipeline/toolchains/esp_idf.py
import os
import threading
import http.server
import socketserver
from ..core.utils import run_cmd
from .base import BaseToolchain


class EspIdfToolchain(BaseToolchain):
    def __init__(self, config, docker_manager):
        super().__init__(config)
        self.docker = docker_manager
        self.image = config.ESP_IMAGE
        self.esp_idf_export = config.ESP_IDF_EXPORT
        self.esp_matter_export = config.ESP_MATTER_EXPORT

    def _execute(self, cmd_str, workdir=None):
        """Executes a command either in Docker or locally based on the config."""
        docker_workdir = workdir.replace(self.config.PROJECT_ROOT, "/project").replace(
            "\\", "/"
        )

        if workdir is None:
            workdir = self.config.WORKDIR

        if getattr(self.config, "USE_DOCKER", True):
            matter_cmd = (
                f"source {self.esp_idf_export} && "
                f"source {self.esp_matter_export} && "
                f"cd {docker_workdir} && "
                f"{cmd_str}"
            )

            self.docker.run_container(
                image=self.image,
                command=matter_cmd,
                workdir=workdir,
            )
        else:
            print(f"💻 Running locally in {workdir}...")
            cmd = ["bash", "-c", cmd_str]
            run_cmd(cmd, cwd=workdir)

    def _build(self, *args, **kwargs):
        # Use args[0] if it exists, otherwise check kwargs, otherwise default to "esp32"
        target = args[0] if args else kwargs.get("target", "esp32")
        
        # Get image_bin from kwargs or use default
        factory_bin = kwargs.get("image_bin", "build/factory.bin")

        # Build command
        build_cmd = (
            f"rm -rf build sdkconfig && "
            f"export IDF_TARGET={target} && "
            f"export SDKCONFIG_DEFAULTS=/project/sdkconfig.defaults && "
            f"idf.py build && "
            f"cd build && esptool.py --chip {target} merge_bin -o {factory_bin} @flash_args"
        )

        # Execute the build command
        self._execute(build_cmd, workdir=self.config.WORKDIR)

    def _clean(self):
        self._execute("idf.py clean", workdir=self.config.WORKDIR)

    def _clean_all(self):
        self._execute("idf.py fullclean", workdir=self.config.WORKDIR)

    def _run_unit_tests(self):
        self._run_in_docker(
            "idf.py set-target linux && idf.py build && ./build/unit_test_app",
            workdir="/project/tests/unit",
        )

    def _flash_usb(self, *args, **kwargs):
        # Parse Args
        port = args[0] if args else kwargs.get("port")
        binary_dir = args[1] if len(args) > 1 else kwargs.get("binary_dir")
        flash_args_filename = args[2] if len(args) > 2 else kwargs.get("flash_args", "flash_args")

        # Path Validation
        if not port or not binary_dir:
            raise ValueError("❌ Missing port or binary_dir")

        # We only check for existence using the full path
        full_args_path = os.path.join(binary_dir, flash_args_filename)
        if not os.path.exists(full_args_path):
            raise FileNotFoundError(f"❌ Could not find {full_args_path}")

        cmd = [
            "python", "-m", "esptool",
            "--chip", self.config.TARGET,
            "--port", port,
            "--baud", "460800",
            "--before", "default-reset",
            "--after", "hard-reset",
            "write-flash",
            f"@{flash_args_filename}" 
        ]
            
        run_cmd(cmd, cwd=binary_dir)

    def _flash_ota(self, port, binary_dir, ota_port):
        class Handler(http.server.SimpleHTTPRequestHandler):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, directory=binary_dir, **kwargs)

        httpd = socketserver.TCPServer(("", ota_port), Handler)
        thread = threading.Thread(target=httpd.serve_forever, daemon=True)
        thread.start()

        run_cmd(
            [
                "pytest",
                os.path.join(self.config.HIL_TEST_DIR, "test_ota_trigger.py"),
                f"--port={port}",
                f"--ota-port={ota_port}",
            ]
        )
        httpd.shutdown()

    def extract_build_artifacts(self, build_dir, extract_dir):
        self._execute(
            f"cp -r {build_dir}/* {extract_dir}/", workdir=self.config.WORKDIR
        )
