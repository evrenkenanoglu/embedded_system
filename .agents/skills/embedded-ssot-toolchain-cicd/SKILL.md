---
name: embedded-ssot-toolchain-cicd
description: >-
  Architectural invariants, SSoT configuration standards, Task-Runner conventions,
  toolchain abstractions, CI/CD pipeline engine, and continuous learnings log for
  Embedded_IoT_BT_WIFI_Base_Project. Activate when modifying or reviewing configurations,
  toolchains, build scripts, CI/CD pipelines, partitioning, and provisioning workflows.
---

# Embedded IoT SSoT & Toolchain Architecture Specification

This specification defines the architectural invariants, configuration hierarchies, toolchain contracts, CI/CD automation rules, and post-mortem error logs for `Embedded_IoT_BT_WIFI_Base_Project`.

---

## 1. Master SSoT Hierarchy & Configuration Rules

### A. The Hierarchy of Authority
1. **Master Project SSoT (`configs/config_project.yaml`)**:
   * Contains all project-wide identity, target silicon, physical hardware ratings (flash size, bus frequencies), cryptographic curves, security tokens, network URLs, and canonical directory paths.
   * Defines the physical flash memory partition table layout under `flash_layout`.
2. **Subsystem Domain Configurations (`configs/config_<domain>.yaml`)**:
   * Must declare `base_config: "config_project.yaml"`.
   * **Zero Literal Duplication**: Must never re-declare values defined in `config_project.yaml` (e.g. `project.name`, `target.chip`, `hardware.flash_size`). All shared fields must use `{section.key}` placeholders.
   * **No Self-Referential Overrides**: Never re-declare a section key with its own name (e.g., `project: { name: "{project.name}" }` in a child config overwrites the base value with a literal string). Inherited keys exist automatically.
3. **CI/CD Pipeline Configurations (`CI_CD/config_*.yaml`)**:
   * `CI_CD/config.yaml`: Common CI baseline referencing `base_config: "../configs/config_project.yaml"`.
   * `CI_CD/config_ci.yaml`: Declarative container build, source checkout, and artifact packaging pipeline.
   * `CI_CD/config_hil.yaml`: Declarative physical hardware flashing and pytest verification pipeline.

### B. Path Token Standards
* All path placeholders must use the `{paths.*}` prefix (e.g., `{paths.workspace_dir}`, `{paths.embedded_system_dir}`, `{paths.build_dir}`, `{paths.certs_dir}`, `{paths.keys_dir}`).

---

## 2. Flash Geometry & Pre-Build SSoT Generation

### A. Partition Table & Kconfig Separation
* **`partitions.csv` (Generated)**: Auto-generated from `flash_layout.partitions` in `config_project.yaml` via `embedded_system/Tasks/build_config.py` (or `prebuild.py`). Never edit `partitions.csv` manually.
* **`sdkconfig.hardware` (Generated Overlay)**: Auto-generated from `hardware` in `config_project.yaml`. Contains flash size (`8MB`), SPI mode (`dio`), frequency (`80m`), partition table offset, and secure boot keys.
* **`sdkconfig.defaults` (Static Base)**: Contains only application feature flags (Matter clusters, FreeRTOS, BLE, mbedTLS, LWIP). All flash geometry, partition offsets, and security keys are stripped out.
* **ESP-IDF Multi-Defaults Merging**: Build commands natively merge both files:
  ```bash
  idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.hardware" build merge-bin -o factory.bin
  ```

### B. Project Version Ingestion
* `PROJECT_VER` and `PROJECT_VER_NUMBER` must be set before `project.cmake` runs in `CMakeLists.txt`.
* The toolchain passes them via `-D` flags from SSoT:
  ```bash
  idf.py -DPROJECT_VER="{project.version}" -DPROJECT_VER_NUMBER={project.version_number} ...
  ```
* In `CMakeLists.txt`, variables are protected by `if(NOT DEFINED ...)` guards.

---

## 3. Toolchain & Task Architecture

### A. Layer Hierarchy
$$\text{Invoke Tasks (Tasks/)} \longrightarrow \text{BaseToolchain (Template/Lifecycle)} \longrightarrow \text{EspIdfToolchain (Vendor Commands)} \longrightarrow \text{idf.py / esptool}$$

1. **`IToolchain` (`embedded_system/Tasks/toolchains/interface.py`)**:
   * Declares only universal embedded operations: `build`, `flash`, `monitor`, `test`, `clean`, `clean_all`.
   * Vendor-specific operations (`menuconfig`, `erase`, `flash_ota`) are excluded from the base interface.
2. **`BaseToolchain` (`embedded_system/Tasks/toolchains/base.py`)**:
   * Implements the Template Method pattern: stage banners, timing via `time.perf_counter()`, TTY color detection (`NO_COLOR` / `FORCE_COLOR` compliance), and exception handling.
   * Delegates default target/port resolution to subclass hooks (`_get_default_target()`, `_get_default_port()`).
3. **`EspIdfToolchain` (`embedded_system/Tasks/toolchains/esp_idf.py`)**:
   * Uses native ESP-IDF v5 `idf.py build merge-bin -o <out_name>` (eliminates `cd build` and manual `esptool.py` calls).
   * Does not source host activation scripts if `/.dockerenv` exists or `IDF_PATH` is already active.
   * Implements ESP-specific methods (`menuconfig`, `erase`, `flash_ota`) directly.
4. **`embedded_system/Tasks/platform.py`**:
   * Platform-agnostic Invoke CLI adapter mounted under `inv es.platform.*`. If `--target` or `--port` is empty, delegates to `IToolchain` hooks and resolves defaults from SSoT in one line.

---

## 4. Configuration Resolution Engine (`Tools/task-runner/core/`)

### A. `config_resolver.py`
* **`resolve_config(config_path)`**: Recursively loads `base_config`, deep-merges dictionaries, and runs up to 5 token expansion passes (`{placeholder.key}`).
* **`resolve_to_file(config_path)`**: Resolves any `config_*.yaml` and writes a flat, standalone temporary YAML file to `build/configs/<name>.resolved.yaml`.
* **`run_with_resolved_config(c, script_path, config_path, ...)`**: Resolves the config to disk, wraps execution with `CommandSerializer`, activates `.venv`, and executes standalone tools with `--config <resolved_file>`.

### B. `config_loader.py`
* Discovers and merges task configurations (`config_tasks.yaml`).
* Populates the global `CONFIG` object with resolved `pathlib.Path` objects.
* Sets up `.venv` environment variables (`PATH`, `VIRTUAL_ENV`, `venv_activate_cmd`).

---

## 5. CI/CD Pipeline Engine (`CI_CD/` & `embedded_system/CI_CD/`)

### A. Declarative Schema (`CI_CD/config_ci.yaml` & `config_hil.yaml`)
* **`type: host`**: Executes filesystem actions (`clean_path`, `delete_file`, `git_checkout`) or shell commands.
* **`type: docker`**: Executes tasks inside containers with directory enforcement and host pip caching.
* **`type: artifact`**: Bundles (`create_package`, `create_zip`) or extracts (`extract_zip`) release archives.

### B. `PipelineEngine` (`embedded_system/CI_CD/pipeline/core/engine.py`)
* Consumes resolved YAML configurations and executes stages sequentially.
* Invoked via CLI tasks: `inv es.ci-cd.ci` and `inv es.ci-cd.hil`.

### C. `DockerManager` (`embedded_system/CI_CD/pipeline/core/docker_manager.py`)
* Formats all volume mounts and working directories to POSIX format (`/project/...`).
* Prepends `cd <container_workdir>` to container shell commands to defeat image entrypoints that change directories to `/opt/espressif/...`.
* Mounts host pip cache (`~/.cache/pip` on Linux/WSL, `%LOCALAPPDATA%/pip/cache` on Windows) to `/root/.cache/pip` in the container to eliminate repetitive network package downloads.
* Implements host-first directory cleanup with fallback to Alpine containers for Docker root-owned files.

---

## 6. Pre-Execution Quality Checklist

Before running or committing changes:
1. [ ] **Flash Invariant**: `hardware.flash_size` in `config_project.yaml` is `"8MB"`, `sdkconfig.hardware` has `CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y`, and `partitions.csv` ends below `0x800000`.
2. [ ] **Version Alignment**: `PROJECT_VER` and `PROJECT_VER_NUMBER` exist in `config_project.yaml` and are passed to `idf.py -D`.
3. [ ] **Zero Hardcoded Paths**: No `/home/...` or `C:\...` strings in Python source files. Paths use `Path` resolution relative to workspace root.
4. [ ] **Tool Purity**: Standalone tools (`generate_pki.py`, `ota-server`, `provision_hardware.py`) load configs via standard `yaml.safe_load()`; task files pass them compiled files via `resolve_to_file()`.
5. [ ] **Task Discovery**: `tasks.py` exports `ns = getattr(task_module, "ns", ...)`. Sub-packages contain `__init__.py`.

---

## 7. Continuous Learnings & Mistakes Log

| ID | Issue / Symptom | Root Cause | Preventive Rule / Invariant |
| :--- | :--- | :--- | :--- |
| **ERR-001** | Duplicate `config.yaml` files across subfolders | Standalone tools created local configs, diverging from root settings. | Delete local tool configs; tools must accept `--config` pointing to resolved SSoT configs. |
| **ERR-002** | Blind `**/config.yaml` recursive glob in task-runner | `config_loader.py` pulled unrelated configs and corrupted root paths. | Explicitly target known config filenames. Ban un-scoped directory globs. |
| **ERR-003** | Auth API Key mismatch between device and OTA server | `config_provisioning.yaml` and `config_ota_server.yaml` hardcoded divergent tokens. | Define tokens once in `config_project.yaml` under `security.api_key` and inherit. |
| **ERR-004** | Cryptographic curve mismatch | PKI generated ECDSA while OTA server configured RSA. | Define curve once in `security.crypto_curve` (`ec-secp256r1`) and inherit. |
| **ERR-005** | Flash size divergence (16MB vs 8MB) | `sdkconfig.defaults` declared 16MB while `config_project.yaml` declared 8MB. | Generate `sdkconfig.hardware` from `config_project.yaml` and merge at build time. |
| **ERR-006** | `ota_image_tool.py: error: argument -vn/--version: expected one argument` | `PROJECT_VER_NUMBER` was missing in CMake, leaving `-vn` empty. | Always inject `-DPROJECT_VER` and `-DPROJECT_VER_NUMBER` via toolchain `build_cmd`. |
| **ERR-007** | `attempted relative import with no known parent package` | Task runner loaded task files directly by file path without package context. | Use absolute imports (`from embedded_system.Tasks...`). Modules must have `_` not `-`. |
| **ERR-008** | `Can't find any collection named 'tasks'!` inside Docker | Docker image entrypoint ran `cd /opt/espressif/esp-matter`, moving out of project mount. | `DockerManager` must explicitly prepend `cd <container_workdir>` to shell commands. |
| **ERR-009** | Empty `tasks.py` causing `CollectionNotFound` | `tasks.py` executed submodule loader but omitted `ns = task_module.ns`. | Ensure `tasks.py` assigns `ns` to expose the Invoke task collection. |
| **ERR-010** | Host activation script failure inside Docker | `EspIdfToolchain` sourced host script containing host-only paths (`/home/...`). | Do not source activation scripts if `/.dockerenv` exists or `IDF_PATH` is already set. |
| **ERR-011** | `FileNotFoundError: 'build/factory.bin'` during `merge_bin` | Toolchain did `cd build` and then told `esptool.py` to write to `build/factory.bin`. | Use native ESP-IDF v5 `idf.py build merge-bin -o <name>` from workspace root. |
| **ERR-012** | Unexpanded `{project.name}` tokens in resolved config | Child config re-declared `project: { name: "{project.name}" }`, overwriting SSoT base values. | Child configs must never redefine inherited keys with self-referential placeholders. |
| **ERR-013** | `/{workspace_dir}/.venv/bin/activate: No such file or directory` | `config_loader.py` passed unexpanded `{workspace_dir}` string into `_resolve_paths()`. | Expand all `{workspace_dir}` tokens across the entire config before converting paths to `Path`. |
| **ERR-014** | `KeyError: 'firmware_dir'` in provisioning `main.py` | `config_provisioning.yaml` paths section omitted `firmware_dir`. | Explicitly map `firmware_dir: "{paths.build_dir}"` in `config_provisioning.yaml`. |
| **ERR-015** | `TypeError: 'str' object cannot be interpreted as an integer` | Token expansion converted numeric values to strings before tool ingestion. | Tools must explicitly cast numeric parameters with `int()` (e.g. `key_size_bytes`, `port`). |
