---
name: standardize-and-scaffold-tool
description: Authoritative skill to inspect, audit, and scaffold any host tool in Tools/ or embedded_system/Tools/. Enforces clean separation between dumb tool CLIs, basic colocated tasks, and SSoT domain orchestrator tasks.
version: 3.1.0
target_runtime: Python 3.13 / uv / invoke
target_project: Embedded_IoT_BT_WIFI_Base_Project
triggers:
  - "standardize tool <path>"
  - "audit tool <path>"
  - "scaffold tool <path>"
  - "bind tool to task runner"
---

# Skill: Tool Standardization & SSoT Scaffold Generator

## 1. Architectural Interaction Model

```
┌────────────────────────────────────────────────────────────────────────┐
│ 1. ROOT TASK-RUNNER (Tools/task-runner/)                               │
│  - Generic, project-agnostic engine.                                   │
│  - Auto-scans all `Tools/**/tasks.py`.                                │
│  - Mounts basic tool tasks under: `inv tools.<tool_name>.<task>`      │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ delegates
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 2. SSoT DOMAIN TASK (embedded_system/Tasks/<tool_name>.py)             │
│  - System-level orchestrator with `@task(help={...})` metadata.        │
│  - Resolves SSoT tokens: `configs/config_<tool>.yaml` -> `build/...`   │
│  - Calls the tool's auto-discovered task with the resolved config.     │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ calls
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 3. COLOCATED TOOL TASK (Tools/<TOOL_NAME>/tasks.py)                    │
│  - Basic Invoke wrapper mirroring `main.py` arguments directly.        │
│  - ZERO SSoT logic, NO `core.config_resolver` imports.                │
│  - Sits right next to `main.py`: `Path(__file__).parent / "main.py"`.  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ executes
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 4. STANDALONE CLI ENGINE (Tools/<TOOL_NAME>/main.py)                   │
│  - Standalone, zero-dependency CLI. Consumes resolved YAML argument.   │
│  - Works offline with local `config.example.yaml`.                     │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. The 5 Invariant Pillars

Every tool must satisfy these 5 pillars. Manual script path registration in `config_tasks.yaml` is **forbidden** (auto-discovery handles detection):

| # | Pillar | Location | Standard Requirement |
| :--- | :--- | :--- | :--- |
| **P1** | **Unified CLI** | `Tools/<TOOL>/main.py` | Accepts config path argument, uses `parse_known_args()`, decoupled from project `core`. |
| **P2** | **Colocated Task** | `Tools/<TOOL>/tasks.py` | Basic Invoke wrapper of `main.py`. Auto-discovered. NO SSoT token resolution. |
| **P3** | **Standalone Config** | `Tools/<TOOL>/config.example.yaml` | Local default values only. NO `{tokens}`, NO `base_config`. |
| **P4** | **Dependencies** | `Tools/<TOOL>/requirements.in` | Direct dependencies declared, compiled to `requirements.txt` via `uv`. |
| **P5** | **SSoT Orchestrator** | `embedded_system/Tasks/<tool>.py` | Standard header, `help={...}` parameter dicts, resolves SSoT tokens, delegates to `inv tools.<tool>.<task>`. |

---

## 3. Tool Archetypes & Remediation Rules

When auditing a directory inside `Tools/`:

### Archetype A: Already Structured (e.g., `FORMATTER/`, `AI_Assistant_Runner/`)
- Has `main.py`, `config.example.yaml`, `requirements.in`.
- **Action**: Add colocated `tasks.py`, ensure `configs/config_<tool>.yaml` exists, and write `embedded_system/Tasks/<tool>.py`.

### Archetype B: Single Ad-Hoc Script (e.g., `PKI/`, `ESP/`, `BUILD_CONFIG/`)
- Contains a flat script like `generate_pki.py` or `get_flash_size.py`.
- **Action**:
  - Keep the original script intact.
  - Create `main.py` as a wrapper that accepts the config file and invokes the original script logic.
  - Generate `tasks.py`, `config.example.yaml`, `requirements.in`, and the SSoT task.

### Archetype C: Multi-Runner Scripts (e.g., `AI_Code_Review/`, `WSL/`, `HW_SECURITY/`)
- Contains multiple scripts (`runner_mr.py`, `runner_freertos.py`, etc.).
- **Action**:
  - Create a unified `main.py` with `--mode <mode>` or sub-commands dispatching to runners.
  - In `tasks.py`, expose matching `@task` functions for each runner.

---

## 4. Canonical Implementation Templates

### Template 1: `Tools/<TOOL_NAME>/main.py` (Dumb CLI Engine)
```python
"""
@file       main.py
@brief      Standalone CLI entry point for <TOOL_NAME>.
@copyright  (c) 2026 - All Rights Reserved
"""

import argparse
import sys
from pathlib import Path
import yaml


def parse_args():
    parser = argparse.ArgumentParser(description="<TOOL_NAME> Engine")
    parser.add_argument(
        "config",
        type=Path,
        help="Path to YAML configuration file",
    )
    return parser.parse_known_args()


def main():
    args, extra_args = parse_args()

    config_file = args.config.resolve()
    if not config_file.is_file():
        sys.stderr.write(f"Error: Configuration file '{config_file}' not found.\n")
        sys.exit(1)

    with open(config_file, "r", encoding="utf-8") as f:
        config = yaml.safe_load(f) or {}

    tool_cfg = config.get("<tool_name>", config)
    print(f"[<TOOL_NAME>] Executing with config: {config_file}")

    # --- Tool business logic here ---
    sys.exit(0)


if __name__ == "__main__":
    main()
```

---

### Template 2: `Tools/<TOOL_NAME>/tasks.py` (Basic Colocated Task Wrapper)
*Notice: Zero SSoT imports, zero `core.config_resolver` logic. It simply mirrors `main.py`.*

```python
"""
@file       tasks.py
@brief      Basic Invoke task wrapper mirroring main.py CLI arguments.
@copyright  (c) 2026 - All Rights Reserved
"""

import sys
from pathlib import Path
from invoke import Context, task


@task(
    help={
        "config": "Path to YAML config file (default: config.example.yaml)",
        "opts": "Forward arbitrary extra flags directly to main.py",
        "dry_run": "Print execution command without running",
    }
)
def run(
    c: Context,
    config: str = "config.example.yaml",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Execute <TOOL_NAME> CLI engine."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "main.py"

    config_path = Path(config)
    if not config_path.is_absolute() and not config_path.exists():
        config_path = (tool_dir / config_path).resolve()

    cmd = f'python "{script_path}" "{config_path}"'
    if opts:
        cmd += f" {opts}"

    pty_enabled = sys.platform != "win32"
    c.run(cmd, dry=dry_run, pty=pty_enabled)
```

---

### Template 3: `Tools/<TOOL_NAME>/config.example.yaml` (Standalone)
```yaml
# ==============================================================================
# Standalone Configuration Template for <TOOL_NAME>
# Usage: python main.py config.example.yaml
# ==============================================================================
<tool_name>:
  output_dir: "output"
  verbose: true
```

---

### Template 4: `configs/config_<tool_name>.yaml` (SSoT Domain Config)
```yaml
base_config: config_project.yaml

<tool_name>:
  output_dir: "{paths.output_dir}/<tool_name>"
  target_chip: "{target.chip}"
  verbose: false
```

---

### Template 5: `embedded_system/Tasks/<tool_name>.py` (SSoT Orchestrator)
*Notice: Canonical header, `@task(help={...})` dicts, resolves SSoT tokens, delegates to `inv tools.<tool>.<task>`.*

```python
"""
@file       <tool_name>.py
@brief      <TOOL_NAME> orchestration task-runner module for interactive CLI sessions.
@copyright  (c) 2026 - All Rights Reserved
"""

import sys
from pathlib import Path
from invoke import Context, task
from core import CONFIG
from core.config_resolver import resolve_to_file


@task(
    help={
        "config": "Explicit path to custom configuration YAML (overrides SSoT)",
        "dry_run": "Print execution command without running",
        "opts": "Forward arbitrary extra flags directly to the tool",
    }
)
def run(
    c: Context,
    config: str = "",
    dry_run: bool = False,
    opts: str = "",
) -> None:
    """Resolve SSoT config and delegate to the tool's basic task."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve SSoT config here
    if config and str(config).strip():
        raw_config = Path(config)
    else:
        raw_config = Path(CONFIG.paths.configs_dir) / "config_<tool_name>.yaml"

    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config = resolve_to_file(raw_config, workspace_root=workspace_root)

    # 2. Delegate to the tool's auto-discovered task
    opts_flag = f'--opts="{opts.strip()}"' if opts else ""
    c.run(
        f'inv tools.<tool_name>.run --config="{resolved_config}" {opts_flag}',
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )
```

---

### Template 6: `Tools/<TOOL_NAME>/requirements.in`
```text
pyyaml>=6.0.2
```
*Lockfile compiled via:* `uv pip compile requirements.in -o requirements.txt`

---

## 5. Verification Protocol

The AI agent must verify the following checklist before completing:

1. **Standalone Test**:
   ```bash
   python Tools/<TOOL_NAME>/main.py Tools/<TOOL_NAME>/config.example.yaml
   ```
2. **Auto-Discovery Check**:
   ```bash
   inv --list
   ```
   *Verify that `tools.<tool_name>.<task>` appears automatically.*
3. **SSoT Orchestrator Dry-Run**:
   ```bash
   inv es.<tool_name>.run --dry-run
   ```
   *Verify that SSoT tokens compile into `build/configs/config_<tool_name>.resolved.yaml` and delegate to the tool task.*
4. **Zero Path Registrations**:
   *Confirm `configs/config_tasks.yaml` was NOT modified with redundant script paths.*
