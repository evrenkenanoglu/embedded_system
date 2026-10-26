---
name: project-automation-cli
description: >-
  Comprehensive specification for the PyInvoke-based task orchestration system.
  Defines the architecture of CLI tasks, Single Source of Truth (SSoT) resolution,
  dynamic task discovery, runtime configuration compilation, toolchain decoupling,
  and strict platform-independence invariants (Windows/WSL/Linux). Activate when
  adding new tasks or task files, modifying build scripts, or updating CI/CD pipelines.
---

# Project Automation & PyInvoke Task Architecture

This skill defines the architecture, dynamic task discovery mechanism, Single Source of Truth (SSoT) resolution, runtime configuration generation, and platform-independence invariants for task orchestration across the `Embedded_IoT_BT_WIFI_Base_Project`.

---

## 1. System Components & File Layout

| Directory / File | Description & Responsibility |
| :--- | :--- |
| `tasks.py` (Workspace Root) | Root PyInvoke entry point. Injects `Tools/task-runner` into `sys.path`, loads the auto-discovery engine, and exposes `ns` to the `inv` CLI. |
| `configs/` | Central SSoT configuration repository (`config_project.yaml` master SSoT + subsystem domain configs: `config_tasks.yaml`, `config_pki.yaml`, `config_provisioning.yaml`, `config_ota_server.yaml`, `config_formatter.yaml`). |
| `Tools/task-runner/` | Task runner engine containing configuration resolution, dynamic module discovery, command serialization, and built-in workspace tasks (`venv`, `vscode`, `test`). |
| `Tools/task-runner/core/` | Engine core: `config_loader.py` (multi-pass token resolution & OS environment), `config_resolver.py` (SSoT inheritance & `resolve_to_file`), `serializer.py` (`CommandSerializer`). |
| `embedded_system/Tasks/` | Domain-specific task modules (`esp32.py`, `crypto.py`, `build_config.py`, `ota.py`, `ci_cd.py`, `deps.py`, `ai.py`, `util.py`, `wsl.py`) and hardware `toolchains/`. |

---

## 2. Dynamic Task Discovery Engine

Task auto-discovery is managed by `Tools/task-runner/tasks/__init__.py`. **Manual import or registration of task modules in `tasks.py` is never required.**

1. **Built-in Tool Tasks**: Scans `Tools/task-runner/tasks/*.py` and registers them at the root CLI namespace (`inv venv.*`, `inv vscode.*`, `inv test.*`).
2. **Root Workspace Tasks**: Scans `WORKSPACE_DIR/Tasks/` or `WORKSPACE_DIR/tasks/` if present.
3. **Subproject Task Discovery**: Recursively scans all subproject directories in the workspace (excluding `IGNORE_DIRS` like `.venv`, `build`, `.git`).
   - If a folder contains a `Tasks/` or `tasks/` directory (e.g., `embedded_system/Tasks/`), it is automatically discovered.
   - Namespace aliases defined in `configs/config_tasks.yaml` under `task_namespaces:` map folder names to CLI prefixes (e.g., `embedded_system: es`).
   - Module filenames inside `Tasks/` form sub-collections (e.g., `embedded_system/Tasks/crypto.py` becomes `inv es.crypto.<task>`).

---

## 3. SSoT Configuration & Runtime Config Generation

### Single Source of Truth (SSoT) Hierarchy
- `configs/config_project.yaml` is the master SSoT containing target hardware chips, baud rates, project metadata, and canonical workspace paths.
- Subsystem YAMLs inherit from master SSoT via `base_config: config_project.yaml` and reference properties using token placeholders (e.g., `{project.name}`, `{paths.configs_dir}`).

### Runtime Generation of Config Files (`resolve_to_file`)
Backend Python tools in `Tools/` or `Source/Scripts/` must remain decoupled and standalone without implementing complex YAML parsing or inheritance logic.

- When a task runs, it calls `core.config_resolver.resolve_to_file(raw_config_path, workspace_root=...)`.
- `resolve_to_file` recursively merges inherited `base_config` files, interpolates all `{placeholder}` tokens, flattens the dictionary, and writes a standalone resolved YAML file to `build/configs/<name>.resolved.yaml`.
- The task passes `--config "build/configs/<name>.resolved.yaml"` to the underlying script or sets environment variables (e.g., `OTA_CONFIG_PATH`).

---

## 4. Platform Independence Invariants (Windows / WSL / Linux)

All tasks and task-runner components must run seamlessly across Windows, WSL, and Linux native environments.

### A. Universal `inv` Execution Wrapper (No Raw Shell Commands)
- Never use raw shell invocations (`idf.py build`, `esptool.py`, `python main.py`) directly in workflows or documentation.
- All operations must be executed through `inv <namespace>.<task>`.

### B. Path Formatting & Manipulation
- **Path Objects**: Always use `pathlib.Path` for path arithmetic. Use the `/` operator rather than string concatenation or hardcoded slashes/backslashes.
- **CLI Command Quoting**: Always quote path parameters in command strings: `f'python "{script_path}" --config "{resolved_config_file}"'`.

### C. Environment Propagation & Path Separators
- Construct `PYTHONPATH` or `PATH` using `os.pathsep` (`:` on Linux/WSL, `;` on Windows):
  ```python
  env_paths = [str(workspace_root), str(task_runner_dir), str(embedded_system_dir)]
  execution_env["PYTHONPATH"] = os.pathsep.join(env_paths)
  ```

### D. Virtual Environment Activation & Command Serialization
- Tasks must execute subprocess commands using `CommandSerializer`:
  ```python
  serializer = CommandSerializer(
      prefix_commands=[CONFIG.venv_activate_cmd],
      env=execution_env,
  )
  serializer.add(cmd)
  serializer.run(c, dry_run=dry_run)
  ```
- `CONFIG.venv_activate_cmd` dynamically selects `call "...ctivate.bat"` on Windows and `source "...ctivate"` on Linux/WSL.
- `CommandSerializer` automatically handles `pty=(not IS_WINDOWS)` to prevent pseudo-terminal errors on Windows.

### E. Stream Encoding Safety
- Environment variables `PYTHONUTF8=1` and `PYTHONIOENCODING=utf-8` are injected automatically to ensure UTF-8 encoding across Windows PowerShell/CMD and Linux terminals.

### F. Hardware Toolchain Decoupling
- Tasks must not import vendor-specific tools (`esptool`, `espefuse`).
- Hardware operations are delegated to `IToolchain` adapters (e.g., `EspIdfToolchain`), retrieved via `get_toolchain(CONFIG)`, encapsulating vendor syntax and flags platform-independently.

---

## 5. Rules for Adding Tasks and Task Files

### Rule 1: Adding a New Task Function to an Existing File
1. Decorate the function with `@task(help={...})`.
2. Ensure the first argument is `c: Context`.
3. Read configurations from `core.CONFIG` or `CONFIG.paths`. Do not hardcode paths or options.
4. If invoking an external script, compile flat runtime config using `resolve_to_file()`.
5. Dispatch commands using `CommandSerializer`.

```python
from pathlib import Path
from invoke import Context, task
from core import CONFIG, CommandSerializer
from core.config_resolver import resolve_to_file

@task(help={"dry_run": "Print command line without executing", "opts": "Forward options"})
def my_task(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Task description displayed in inv --list."""
    script_path = Path(CONFIG.paths.my_script).resolve()
    resolved_config = resolve_to_file(CONFIG.paths.my_config, workspace_root=CONFIG.paths.workspace_dir)

    serializer = CommandSerializer(prefix_commands=[CONFIG.venv_activate_cmd], env=CONFIG.env)
    serializer.add(f'python "{script_path}" --config "{resolved_config}"', extra=opts)
    serializer.run(c, dry_run=dry_run)
```

### Rule 2: Adding a New Task File (Module)
1. Place the Python file inside `embedded_system/Tasks/<module_name>.py` (or under any subproject's `Tasks/` directory).
2. Name the file using `snake_case` (e.g., `system_health.py`).
3. **Do NOT edit `tasks.py`**. The dynamic auto-discovery engine automatically scans `Tasks/` directories and loads all `.py` files.
4. Tasks inside `system_health.py` will be automatically exposed under `inv es.system-health.<task_name>`.

### Rule 3: Adding a New Task Directory / Subproject
1. Create a `Tasks/` folder inside the target subproject directory (e.g., `my_subproject/Tasks/`).
2. Add `.py` task modules inside `my_subproject/Tasks/`.
3. (Optional) Register a short CLI namespace alias in `configs/config_tasks.yaml` under `task_namespaces:`:
   ```yaml
   task_namespaces:
     embedded_system: es
     my_subproject: sub
   ```
4. All tasks will be auto-discovered and mapped under `inv sub.<module_name>.<task_name>`.
