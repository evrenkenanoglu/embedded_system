---
name: embedded-tool-standardization
description: Comprehensive standard for designing, scaffolding, and integrating host tools into the task-runner ecosystem. Enforces strict decoupling between dumb CLI engines, basic standalone tool tasks, and SSoT domain orchestrator tasks. Includes historical error registry and post-mortem failure prevention rules.
version: 4.0.0
target_runtime: Python 3.13 / uv / invoke
target_project: Embedded_IoT_BT_WIFI_Base_Project
triggers:
  - "create a tool"
  - "standardize tool <path>"
  - "scaffold tool <path>"
  - "bind tool to task runner"
  - "fix task runner error"
---

# Skill: Embedded Tool Architecture, Scaffolding & Error Prevention

## 1. Architectural Mental Model (The 3-Tier Decoupled Flow)

Every tool and task in this ecosystem strictly separates responsibilities into 3 distinct tiers:

```
┌────────────────────────────────────────────────────────────────────────┐
│ TIER 1: SSoT Domain Task (embedded_system/Tasks/<domain>.py)           │
│  - System-level orchestrator mounted under `inv es.<domain>.<task>`    │
│  - Imports `core.config_resolver.resolve_to_file` and `core.CONFIG`.   │
│  - Compiles tokens into `build/configs/<tool>.resolved.yaml`.          │
│  - DELEGATES to the tool's Invoke task via runtime CLI call.           │
│  - NEVER references or executes raw python script filepaths!           │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ delegates: `inv tools.<tool>.<task>`
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ TIER 2: Colocated Standalone Task (embedded_system/Tools/<TOOL>/tasks.py)│
│  - Basic Invoke wrapper mirroring `main.py` CLI arguments.             │
│  - 100% INDEPENDENT: Zero imports from `task-runner` or `core`.        │
│  - Knows its own script locally: `Path(__file__).parent / "main.py"`.  │
│  - Invokes `main.py` using `sys.executable` (never hardcoded 'python').│
│  - Auto-discovered and mounted directly under `tools.<tool-slug>.*`.   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ executes: `python main.py <config>`
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ TIER 3: Standalone CLI Engine (embedded_system/Tools/<TOOL>/main.py)   │
│  - Standalone, zero-dependency Python CLI engine.                      │
│  - Accepts a YAML config file path as its primary input argument.      │
│  - Uses `parser.parse_known_args()` to prevent flag-swallowing bugs.   │
│  - Works 100% offline with local `config.example.yaml`.                │
│  - NEVER imports project SSoT, NEVER crawls parent directories.        │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Dynamic Discovery & SSoT Convention Rules

### Rule A: Subsystem Submodule Autonomy (`embedded_system/config_tasks.yaml`)
Subsystems must declare their own task discovery rules anchored to `{config_dir}` so they work both inside the parent project and when cloned/run standalone:

```yaml
# embedded_system/config_tasks.yaml
paths:
  embedded_system_dir: '{config_dir}'
  es_tools_dir: '{embedded_system_dir}/Tools'
  es_tasks_dir: '{embedded_system_dir}/Tasks'

discovery:
  tasks:
    - path: '{es_tasks_dir}'
      namespace: "es"
      mode: "file"
      pattern: "*.py"

    - path: '{es_tools_dir}'
      namespace: "tools"
      mode: "flat"
      task_filename: "tasks.py"
```

### Rule B: Zero Script Paths in YAML
No file path ending in `.py` (e.g. `*_script`) should ever be registered in `configs/config_tasks.yaml`. All tools are discovered dynamically by scanning `**/tasks.py`.

### Rule C: Stable Flat Tool Namespacing
Regardless of where a tool is stored on disk (e.g., `Tools/AI/AI_Assistant_Runner/tasks.py` vs `Tools/AI_Assistant_Runner/tasks.py`), it must **always** mount flat under `tools.<tool-slug>.*` (e.g., `tools.ai-assistant-runner.start`). Disk folder nesting (`AI/`, `HW_SECURITY/`) must never leak into the Invoke command name.

---

## 3. Production Code Templates

### Template 1: Tier 3 — Standalone CLI Engine (`Tools/<TOOL>/main.py`)
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
    parser = argparse.ArgumentParser(description="<TOOL_NAME> Standalone Engine")
    parser.add_argument(
        "config",
        type=Path,
        help="Path to YAML configuration file",
    )
    # CRITICAL: Use parse_known_args() so extra flags don't break execution
    return parser.parse_known_args()


def main():
    args, extra_args = parse_args()

    config_file = args.config.resolve()
    if not config_file.is_file():
        sys.stderr.write(f"Error: Configuration file '{config_file}' not found.\n")
        sys.exit(1)

    try:
        with open(config_file, "r", encoding="utf-8") as f:
            config = yaml.safe_load(f) or {}
    except Exception as e:
        sys.stderr.write(f"Error reading YAML: {e}\n")
        sys.exit(1)

    tool_cfg = config.get("<tool_name>", config)
    print(f"[<TOOL_NAME>] Executing with config: {config_file}")

    # Core business logic execution here
    sys.exit(0)


if __name__ == "__main__":
    main()
```

---

### Template 2: Tier 2 — Colocated Tool Task (`Tools/<TOOL>/tasks.py`)
*Zero `core` imports. Uses `sys.executable`.*

```python
"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for <TOOL_NAME>.
@note       Completely independent of task-runner and core.
"""

import sys
from pathlib import Path
from invoke import Context, task

# Optional: explicit anchor if tool folder is renamed on disk
TOOL_NAME = "<tool-slug>"


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
    """Execute <TOOL_NAME> standalone engine."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "main.py"

    config_path = Path(config)
    if not config_path.is_absolute() and not config_path.exists():
        config_path = (tool_dir / config_path).resolve()

    # CRITICAL: Always use sys.executable (never raw 'python')
    cmd = f'"{sys.executable}" "{script_path}" "{config_path}"'
    if opts:
        cmd += f" {opts}"

    pty_enabled = (sys.platform != "win32")
    c.run(cmd, dry=dry_run, pty=pty_enabled)
```

---

### Template 3: Tier 1 — SSoT Domain Task (`embedded_system/Tasks/<tool>.py`)
*Canonical header, parameter `config_path` (never `config`), resolves tokens, delegates via Invoke.*

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
        "config_path": "Explicit path to custom configuration YAML (overrides SSoT)",
        "opts": "Forward arbitrary extra flags directly to the tool",
        "dry_run": "Print execution command without running",
    }
)
def run(
    c: Context,
    config_path: str = "",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Resolve SSoT config and delegate to the tool's basic task."""
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve SSoT config at this level
    if config_path and str(config_path).strip():
        raw_config = Path(config_path)
    else:
        raw_config = Path(CONFIG.paths.configs_dir) / "config_<tool_name>.yaml"

    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    resolved_config = resolve_to_file(raw_config, workspace_root=workspace_root)

    # 2. Delegate to the tool's runtime task (zero raw python script paths)
    opts_flag = f'--opts="{opts.strip()}"' if opts and str(opts).strip() else ""
    c.run(
        f'inv tools.<tool-slug>.run --config="{resolved_config}" {opts_flag}',
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )
```

---

## 4. Post-Mortem & Error Registry (Crucial for AI Self-Correction)

This section documents real historical bugs, exact error signatures, root causes, and mandatory preventive rules.

### Incident 1: Invoke Reserved Global Flag Collision
- **Error Signature:**
  ```text
  AttributeError: No attribute or config key found for '_load_'
  invoke.exceptions.UnknownFileType: Config files of type '' (from file '') are not supported!
  ```
- **Trigger:** Defining an Invoke task parameter as `config: str = ""` and running `inv <task> --config=`.
- **Root Cause:** `--config` (or `-f`) is a **reserved global flag in Invoke itself** for loading `invoke.yaml`. When passed an empty string `--config=`, Invoke attempts to parse `""` as an internal config file, fails to detect the extension, and crashes before entering task code.
- **Invariant Rule:** **NEVER name an Invoke task parameter `config`. Always use `config_path` or `cfg`.**

---

### Incident 2: Path-Dependent Tool Task Disappearance
- **Error Signature:**
  ```text
  No idea what 'tools.<tool-name>.generate' is!
  ```
- **Trigger:** Reorganizing a tool on disk (e.g., placing it in `Tools/AI/AI_Assistant_Runner/`) causes the task runner to register `tools.ai.ai_assistant_runner.generate` instead of `tools.ai-assistant-runner.generate`.
- **Root Cause:** Discoverer recursively split the full directory path (`AI/AI_Assistant_Runner`), creating nested Invoke collections reflecting disk depth rather than tool identity.
- **Invariant Rule:** Tool discovery must be **flat under `tools`**. The collection name is derived strictly from `path.parent.name` or an explicit `TOOL_NAME` attribute, slugified to kebab-case. Intermediate folders (`AI/`, `HW_SECURITY/`) must be ignored.

---

### Incident 3: Missing System Binary on POSIX / WSL
- **Error Signature:**
  ```text
  /bin/bash: line 1: python: command not found (exit code 127)
  ```
- **Trigger:** Calling `c.run(f'python "{script}" ...')` inside a task on modern Ubuntu/Debian/WSL.
- **Root Cause:** Modern Linux systems ship with `python3`. The command `python` does not exist in standard subshells unless explicitly symlinked or activated.
- **Invariant Rule:** **Never hardcode `"python"`. Always use `f'"{sys.executable}" "{script}"'`** to guarantee execution inside the active virtual environment.

---

### Incident 4: Subprocess Dropped Enter Keystroke on POSIX
- **Error Signature:** User types prompt into an interactive CLI/TUI (Copilot, Claude Code, serial monitor) launched via Invoke, but pressing `Enter` fails to submit or does nothing.
- **Root Cause:** Invoke defaults to `pty=False`, wrapping `stdin` in non-interactive pipes. Node.js/React Ink TUIs require a raw pseudo-terminal (PTY) to register `\r` (Carriage Return). In a pipe, `Enter` sends `\n` and is ignored.
- **Invariant Rule:** When launching interactive CLI tools via Invoke, **always pass `pty=(sys.platform != "win32")`**. In `main.py`, prefer `os.execvpe` on POSIX to replace the Python wrapper process directly.

---

### Incident 5: CLI Argument Swallowing via `REMAINDER`
- **Error Signature:**
  ```text
  error: unexpected argument '--tool' found
  ```
- **Trigger:** Running `main.py <config> --tool copilot` when `extra_args` was defined using `nargs=argparse.REMAINDER`.
- **Root Cause:** In Python's `argparse`, `REMAINDER` greedily consumes all positional and optional flags that follow the first positional argument, passing `--tool` into the child tool instead of `main.py`.
- **Invariant Rule:** **Never use `nargs=REMAINDER` when forwarding arguments. Always use `parser.parse_known_args()`.**

---

### Incident 6: Parent Directory Crawling in Standalone Tools
- **Error Signature:**
  ```text
  NameError: name 'project_root' is not defined
  FileNotFoundError: .../configs/config_project.yaml not found
  ```
- **Trigger:** `main.py` attempting to detect `.git`, `parents[3]`, or load master project configs.
- **Root Cause:** Violating the "Dumb Tool" boundary. A tool engine must never know it is part of a larger monorepo.
- **Invariant Rule:** **`main.py` must accept an already-resolved config path as an argument.** If running standalone, it uses its local `config.example.yaml`. It never crawls parent directories.

---

### Incident 7: Standalone Submodule Path Collision
- **Error Signature:**
  ```text
  FileNotFoundError: .../embedded_system/embedded_system/Tasks not found
  ```
- **Trigger:** Running `cd embedded_system && inv es.ota.release` when `config_tasks.yaml` had `embedded_system_dir: '{workspace_dir}/embedded_system'`.
- **Root Cause:** When running standalone, `{workspace_dir}` becomes `embedded_system`, leading to double nesting.
- **Invariant Rule:** Subsystem configs must anchor paths using **`{config_dir}`** (e.g. `embedded_system_dir: '{config_dir}'`), ensuring identical resolution whether running from root or standalone.

---

## 5. Verification & Self-Correction Checklist

Before declaring any tool integration complete, execute this 4-step verification:

- [ ] **1. Standalone Execution Test:**
  ```bash
  cd embedded_system/Tools/<TOOL_DIR>
  python main.py config.example.yaml
  ```
  *(Must succeed with zero references to root project configs or `core`).*

- [ ] **2. Tool Task Isolation Test:**
  ```bash
  inv tools.<tool-slug>.run --config=config.example.yaml
  ```
  *(Must execute via the tool's local `tasks.py` without errors).*

- [ ] **3. Dynamic Auto-Discovery Test:**
  ```bash
  inv --list
  ```
  *(Verify that `tools.<tool-slug>.<task>` appears under the flat `tools` namespace).*

- [ ] **4. SSoT Orchestrator Dry-Run Test:**
  ```bash
  inv es.<tool-name>.run --dry-run
  ```
  *(Verify SSoT tokens compile into `build/configs/` and delegate cleanly via `inv tools.<tool-slug>.*`).*
