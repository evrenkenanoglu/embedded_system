# Skill: Task SSoT & Auto-Discovery Architecture

This skill defines the contract for configuration management and automated task discovery in this workspace.

---

## 1. System Architecture & Auto-Discovery Pipeline

The task orchestration system eliminates manual task registration. It relies on a three-tier auto-discovery pipeline:

```text
[ configs/config_project.yaml ] (Master SSoT: Silicon, Version, Paths, Layout)
               ▲
               │ base_config inheritance
[ configs/config_tasks.yaml ] + Discovered [ **/config_tasks.yaml ]
               │
               ▼
[ Tools/task-runner/core/config_loader.py ]
   ├── Resolves {token} substitutions across all configs
   ├── Converts all paths to absolute pathlib.Path objects
   └── Exports global singleton: CONFIG
               │
               ▼
[ tasks.py (Root Auto-Discovery Engine) ]
   ├── Reads CONFIG.task_namespaces (e.g., embedded_system: es)
   ├── Scans <subsystem>/Tasks/*.py (e.g., embedded_system/Tasks/)
   ├── Loads modules via Collection.from_module() into namespace 'es'
   └── Loads runner modules (Tools/task-runner/tasks/ -> test, venv, vscode)
               │
               ├──► inv <namespace>.<module>.<task>
               ├──► inv test.dry-run (Auto-discovers and tests all tasks)
               └──► inv vscode.sync  (Auto-generates .vscode/tasks.json)
```

### Auto-Discovery Rules
1. **Zero Registration**: Developers never touch `tasks.py` or manually write `Collection.add_task()`.
2. **Sub-Config Auto-Discovery**: `config_loader.py` searches for all `**/config_tasks.yaml` across the workspace (excluding `.git`, `.venv`, and `Tools/task-runner`), deep-merging them into `CONFIG`.
3. **Namespace Mapping**: Any directory mapped under `task_namespaces` in `config_tasks.yaml` (e.g., `embedded_system: es`) automatically exposes every `.py` file inside its `Tasks/` subfolder as an Invoke sub-collection:
   - `embedded_system/Tasks/platform.py` $\rightarrow$ `es.platform.<task>`
   - `embedded_system/Tasks/ota.py` $\rightarrow$ `es.ota.<task>`
   - `embedded_system/Tasks/crypto.py` $\rightarrow$ `es.crypto.<task>`
4. **Tool-Runner Tasks**: Modules in `Tools/task-runner/tasks/` (`test.py`, `venv.py`, `vscode.py`) are mounted directly at the root namespace.

---

## 2. Configuration Resolution Contract

### SSoT Token Inheritance Rules
- Domain configs reference the master via `base_config: config_project.yaml`.
- Tokens in the form `{section.property}` or `{paths.key}` are resolved iteratively up to 5 passes by `config_resolver.py`.
- In python task code, **all paths in `CONFIG.paths` are already resolved to absolute `pathlib.Path` objects**.

### Configuration Access Hierarchy
```python
from core import CONFIG

# Project metadata
chip = CONFIG.target.chip                  # 'esp32c6'
binary = CONFIG.project.binary_name        # 'Embedded_IoT_BT_WIFI_Base_Project.bin'

# Hardware defaults
port = CONFIG.hardware.default_port        # '/dev/ttyACM0'
baud = CONFIG.hardware.flash_baud          # 460800

# Canonical resolved paths (pathlib.Path instances)
build_dir = CONFIG.paths.build_dir         # Path('/.../build')
manifest = CONFIG.paths.server_manifest    # Path('/.../release-catalog/manifest.json')
script = CONFIG.paths.provision_hardware_script
```

---

## 3. How Tasks Shall Be Added

### Scenario A: Adding a Task to an Existing Module
To add a new task to an existing domain (e.g., `ota.py` or `platform.py`), append the function directly in `embedded_system/Tasks/<module>.py`:

```python
from pathlib import Path
from typing import Literal
from invoke import Context, task
from core import CONFIG, CommandSerializer

@task(
    help={
        "mode": "Execution profile: 'fast' (differential) or 'full'",
        "port": "Serial port override (SSoT default: hardware.default_port)",
        "dry_run": "Display generated shell commands without executing",
    }
)
def verify(
    c: Context,
    mode: Literal["fast", "full"] = "fast",
    port: str = "",
    dry_run: bool = False,
) -> None:
    """Run hardware verification suite against connected silicon."""
    # 1. Fallback to SSoT if parameter not provided via CLI
    target_port = port or CONFIG.hardware.default_port

    # 2. Build commands using SSoT paths and properties
    cmd = (
        f'python "{CONFIG.paths.provision_hardware_script}" '
        f"--port {target_port} "
        f"--chip {CONFIG.target.chip} "
        f"--mode {mode}"
    )

    # 3. Serialize and execute via CommandSerializer
    serializer = CommandSerializer()
    serializer.add(cmd)
    serializer.run(c, dry_run=dry_run)
```
*Result*: Instantly discovered as `inv es.<module>.verify`.

---

### Scenario B: Adding an Entirely New Task Domain / Module
To add a new domain under the existing `es` namespace:
1. Create `embedded_system/Tasks/<new_domain>.py`.
2. Add tasks decorated with `@task`.
3. If new script paths or tools are needed, add their references into `configs/config_tasks.yaml` under `paths:`.
4. *Result*: The root auto-loader will immediately detect `<new_domain>.py` and expose `inv es.<new_domain>.<task>`.

---

### Scenario C: Adding a New Subsystem Namespace
To add a new subsystem outside of `embedded_system` (e.g., `test_harness` or `gateway`):
1. Create `<subsystem>/Tasks/`.
2. In `configs/config_tasks.yaml` (or in a new `<subsystem>/config_tasks.yaml`), register the namespace:
   ```yaml
   task_namespaces:
     embedded_system: es
     test_harness: th      # Maps test_harness/Tasks/*.py to 'th.*'
   ```
3. Place `.py` files inside `<subsystem>/Tasks/`.
4. *Result*: Auto-discovery mounts them as `inv th.<module>.<task>`.

---

## 4. Parameter Typing & VS Code Synchronization Contract

`Tools/task-runner/tasks/vscode.py` inspects `@task` signatures to build interactive inputs for `.vscode/tasks.json`. Task authors must follow these strict typing patterns:

| Parameter Type | Python Type Annotation | VS Code Input Generated |
| :--- | :--- | :--- |
| **Enum / Choice** | `mode: Literal["prod", "dev"] = "dev"` | `pickString` dropdown |
| **String Input** | `target: str = "app"` | `promptString` dialog with default |
| **Numeric Input** | `baud: int = 115200` | `promptString` dialog with default |
| **Boolean Flag** | `clean: bool = False` | **Omitted** from input prompt (invoked as `--clean`) |
| **Context** | `c: Context` | **Omitted** (Invoke context) |
| **Dry Run** | `dry_run: bool = False` | **Omitted** (Standard execution bypass switch) |

---

## 5. Command Execution Guardrails (`CommandSerializer`)

All task executions must pass through `CommandSerializer`:

```python
serializer = CommandSerializer(
    prefix_commands=[CONFIG.venv_activate_cmd],  # Optional venv activation
    pty=True                                     # Automatic Windows / Linux handling
)
serializer.add(f'idf.py -p {port} flash')
serializer.run(c, dry_run=dry_run)
```

- **Never** invoke raw `subprocess.run()`, `os.system()`, or direct `c.run()` without `--dry-run` checks.
- When `dry_run=True`, `serializer.run()` logs the command sequence and skips execution, enabling automated CI smoke-testing.

---

## 6. Verification Checklist for New Tasks

Whenever a task is authored or changed:

```bash
# 1. Verify auto-discovery and CLI signature
inv --list

# 2. Run automated dry-run smoke tests (validates parameter defaults and imports)
inv test.dry-run

# 3. Synchronize VS Code tasks and inputs
inv vscode.sync
```
