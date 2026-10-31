---
name: embedded-tool-standardization
description: Comprehensive standard for designing, scaffolding, auditing, and integrating host tools into the task-runner ecosystem. Enforces strict 3-tier decoupling between dumb CLI engines, basic standalone tool tasks, and SSoT domain orchestrator tasks. Includes historical error registry and continuous autonomous self-healing rules.
version: 5.0.0
target_runtime: Python 3.13 / uv / invoke
target_project: Embedded_IoT_BT_WIFI_Base_Project
triggers:
  - "create a tool"
  - "create a host tool"
  - "implement utility tool"
  - "standardize tool <path>"
  - "audit tool <path>"
  - "scaffold tool <path>"
  - "bind tool to task runner"
  - "add new tool to embedded_system/Tools"
  - "fix task runner error"
---

# Skill: Host Tool Architecture, Standardization & SSoT Orchestration

## 1. Architectural Mental Model (The 3-Tier Decoupled Flow)

Every host-side utility tool in this ecosystem strictly separates responsibilities into three isolated tiers:

```text
┌────────────────────────────────────────────────────────────────────────┐
│ TIER 1: SSoT Domain Task (embedded_system/Tasks/<domain>.py)           │
│  - System-level orchestrator mounted under `inv es.<domain>.<task>`.   │
│  - Imports `core.config_resolver.resolve_to_file` and `core.CONFIG`.   │
│  - Compiles SSoT tokens into `build/configs/<tool>.resolved.yaml`.     │
│  - DELEGATES to the tool's Invoke task via runtime CLI call.           │
│  - NEVER references or executes raw python script filepaths!           │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ delegates: `inv tools.<tool-slug>.<task>`
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ TIER 2: Colocated Standalone Task (embedded_system/Tools/<TOOL>/tasks.py)│
│  - Basic Invoke wrapper mirroring `main.py` CLI arguments.             │
│  - 100% INDEPENDENT: Zero imports from `task-runner` or `core`.        │
│  - Knows its own script locally: `Path(__file__).parent / "main.py"`.  │
│  - Invokes `main.py` using `sys.executable` (never hardcoded 'python').│
│  - Auto-discovered and mounted flat under `tools.<tool-slug>.*`.       │
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

## 2. The 5 Invariant Pillars & Guardrails

Every tool must satisfy the 5 architectural pillars without exception:

| # | Pillar | Location | Standard Requirement |
| :--- | :--- | :--- | :--- |
| **P1** | **Unified CLI** | `Tools/<TOOL>/main.py` | Accepts config path argument, uses `parse_known_args()`, decoupled from project `core`. |
| **P2** | **Colocated Task** | `Tools/<TOOL>/tasks.py` | Basic Invoke wrapper of `main.py`. Auto-discovered. Zero SSoT token resolution. |
| **P3** | **Standalone Config**| `Tools/<TOOL>/config.example.yaml` | Local default values only. Zero `{tokens}`, zero `base_config`. |
| **P4** | **Dependencies** | `Tools/<TOOL>/requirements.in` | Direct dependencies declared, compiled to lockfile `requirements.txt` via `uv`. |
| **P5** | **SSoT Orchestrator**| `embedded_system/Tasks/<tool>.py` | Standard header, `help={...}` dicts, resolves SSoT tokens, delegates to `inv tools.<tool>.<task>`. |

### Absolute Prohibitions (Anti-Patterns)
1. ❌ **NO Hardcoded Script Paths in YAML:** Never register `<tool>_script` or `<tool>_runner_script` in `config_tasks.yaml`. All tools are discovered dynamically by scanning `**/tasks.py`.
2. ❌ **NO Direct Script Execution from `Tasks/`:** `embedded_system/Tasks/<tool>.py` must never execute `python <path>/main.py`. It resolves SSoT tokens and delegates via `inv tools.<tool-slug>.<task>`.
3. ❌ **NO SSoT Imports in `Tools/`:** Never import `core`, `CONFIG`, or `core.config_resolver` inside `embedded_system/Tools/<TOOL_NAME>/` or its `tasks.py`.
4. ❌ **NO Reserved Invoke Parameter Names:** Never name an Invoke task argument `config` (collides with Invoke's global `--config` flag). Always use `config_path`.
5. ❌ **NO Hardcoded Python Strings:** Never use `f'python "{script}"'`. Always use `f'"{sys.executable}" "{script}"'` to guarantee virtual environment execution on POSIX/WSL.
6. ❌ **NO Directory Traversal:** `main.py` must never crawl parent directories (`parents[3]`, `.git`). It reads *only* the config path passed to it.
7. ❌ **NO Greedy CLI Parsing:** Never use `nargs=argparse.REMAINDER`. Always use `parser.parse_known_args()`.

### Mandatory Requirements
1. ✔️ **Subsystem Autonomy (`embedded_system/config_tasks.yaml`):** Subsystems must anchor discovery paths to `{config_dir}` so they resolve identically whether invoked from root or standalone:
   ```yaml
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
2. ✔️ **Flat Tool Namespacing:** Regardless of folder depth (e.g. `Tools/AI/AI_Assistant_Runner/`), tools must always mount flat under `tools.<tool-slug>.*`. Intermediate directories are stripped from the CLI command name.
3. ✔️ **Interactive vs. Batch Process Separation:** For interactive tools (TUI, REPL, chat, serial monitors), pass `pty=(sys.platform != "win32")` in Invoke, and prefer `os.execvpe` on POSIX in `main.py`.

---

## 3. Canonical File Tree Specification

```text
Embedded_IoT_BT_WIFI_Base_Project/
├── configs/
│   ├── config_tasks.yaml                       # Root workspace settings (NO tool script paths)
│   └── config_<tool_name>.yaml                 # SSoT domain configuration
├── embedded_system/
│   ├── config_tasks.yaml                       # Subsystem discovery rules (anchored to {config_dir})
│   ├── Tasks/
│   │   └── <tool_name>.py                      # Tier 1: SSoT Orchestrator task (resolves tokens, delegates to inv)
│   └── Tools/
│       └── <TOOL_NAME>/                        # Tier 2 & 3: Standalone Tool Directory
│           ├── src/
│           │   ├── __init__.py
│           │   └── ...                         # Core business logic / adapters
│           ├── config.example.yaml             # Standalone local config (NO tokens, NO base_config)
│           ├── main.py                         # Tier 3: Dumb CLI entry point
│           ├── tasks.py                        # Tier 2: Colocated basic Invoke task wrapper
│           ├── requirements.in                 # Unpinned dependencies
│           ├── requirements.txt                # Lockfile compiled via uv
│           └── README.md                       # CLI usage & documentation
```

---

## 4. Tool Archetypes & Migration / Audit Rules

When auditing or scaffolding a tool under `Tools/`:

### Archetype A: Already Structured (e.g., `FORMATTER/`, `AI_Assistant_Runner/`)
- Has `main.py`, `config.example.yaml`, `requirements.in`.
- **Action:** Add colocated `tasks.py`, ensure `configs/config_<tool>.yaml` exists, and create `embedded_system/Tasks/<tool>.py`.

### Archetype B: Single Ad-Hoc Script (e.g., `PKI/`, `BUILD_CONFIG/`)
- Contains a flat script like `generate_pki.py` or `generate_artifacts.py`.
- **Action:**
  1. Keep the original script intact.
  2. Create `main.py` as a wrapper that accepts the config file and invokes the original logic.
  3. Generate `tasks.py`, `config.example.yaml`, `requirements.in`, and the SSoT task in `Tasks/`.

### Archetype C: Multi-Runner Scripts (e.g., `AI_Code_Review/`, `WSL/`, `HW_SECURITY/`)
- Contains multiple scripts (`runner_mr.py`, `runner_freertos.py`, etc.).
- **Action:**
  1. Create a unified `main.py` with `--mode <mode>` or sub-commands dispatching to runners.
  2. In `tasks.py`, expose matching `@task` functions for each runner.

---

## 5. Production Code Templates

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
    # CRITICAL: Use parse_known_args() so forwarded flags do not break execution
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
        sys.stderr.write(f"Error reading YAML '{config_file}': {e}\n")
        sys.exit(1)

    tool_cfg = config.get("<tool_name>", config)
    print(f"[<TOOL_NAME>] Executing with config: {config_file}")

    # Core engine logic here
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

### Template 3: Tier 1 — SSoT Domain Task (`embedded_system/Tasks/<tool_name>.py`)
*Canonical header, parameter `config_path` (never `config`), resolves tokens via `resolve_to_file`, delegates via Invoke.*

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

### Template 4: Standalone Template (`Tools/<TOOL>/config.example.yaml`)
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

### Template 5: SSoT Domain Configuration (`configs/config_<tool_name>.yaml`)
```yaml
base_config: config_project.yaml

<tool_name>:
  output_dir: "{paths.output_dir}/<tool_name>"
  target_chip: "{target.chip}"
  verbose: false
```

---

### Template 6: Dependency Specification (`Tools/<TOOL>/requirements.in`)
```text
pyyaml>=6.0.2
```
*Lockfile compiled via:*
```bash
uv pip compile requirements.in -o requirements.txt
```

---

## 6. Post-Mortem & Error Registry (Mandatory Self-Correction Rules)

### Incident 1: Invoke Reserved Global Flag Collision
- **Error Signature:**
  ```text
  AttributeError: No attribute or config key found for '_load_'
  invoke.exceptions.UnknownFileType: Config files of type '' (from file '') are not supported!
  ```
- **Trigger:** Defining an Invoke task parameter as `config: str = ""` and running `inv <task> --config=`.
- **Root Cause:** `--config` (or `-f`) is a **reserved global flag in Invoke itself** for loading `invoke.yaml`. Passing an empty string crashes Invoke before task execution begins.
- **Invariant Rule:** **NEVER name an Invoke task parameter `config`. Always use `config_path` or `cfg`.**

---

### Incident 2: Path-Dependent Tool Task Disappearance
- **Error Signature:**
  ```text
  No idea what 'tools.<tool-name>.run' is!
  ```
- **Trigger:** Reorganizing a tool on disk (e.g. `Tools/AI/AI_Assistant_Runner/`) causes Invoke to register `tools.ai.ai_assistant_runner.run`.
- **Root Cause:** Discoverer recursively split the directory path, creating nested collections reflecting folder depth rather than tool identity.
- **Invariant Rule:** Tool discovery must be **flat under `tools`**. The command is always `tools.<tool-slug>.<task>`, derived strictly from `path.parent.name` or explicit `TOOL_NAME`.

---

### Incident 3: Missing System Binary on POSIX / WSL
- **Error Signature:**
  ```text
  /bin/bash: line 1: python: command not found (exit code 127)
  ```
- **Trigger:** Calling `c.run(f'python "{script}" ...')` on modern Linux/WSL.
- **Root Cause:** Modern distributions ship with `python3`; `python` is unaliased.
- **Invariant Rule:** **Always use `f'"{sys.executable}" "{script}"'`** to guarantee execution inside the active `.venv`.

---

### Incident 4: Subprocess Dropped Enter Keystroke on POSIX
- **Error Signature:** Interactive CLI/TUI tools (chat, serial monitor) drop `Enter` key presses.
- **Root Cause:** Invoke defaults to `pty=False`. Node.js/React Ink TUIs require raw mode PTY (`\r` vs `\n`).
- **Invariant Rule:** When launching interactive CLI tools via Invoke, **always pass `pty=(sys.platform != "win32")`**. In `main.py`, prefer `os.execvpe` on POSIX to replace the process wrapper.

---

### Incident 5: CLI Argument Swallowing via `REMAINDER`
- **Error Signature:**
  ```text
  error: unexpected argument '--tool' found
  ```
- **Trigger:** Running `main.py <config> --tool copilot` when `extra_args` was defined with `nargs=argparse.REMAINDER`.
- **Root Cause:** `argparse.REMAINDER` greedily consumes all options following the first positional argument.
- **Invariant Rule:** **Never use `nargs=REMAINDER`. Always use `parser.parse_known_args()`.**

---

### Incident 6: Parent Directory Crawling in Standalone Tools
- **Error Signature:**
  ```text
  NameError: name 'project_root' is not defined
  FileNotFoundError: .../configs/config_project.yaml not found
  ```
- **Trigger:** `main.py` attempting to detect `.git`, inspect `parents[3]`, or load master project configs.
- **Root Cause:** Violating the "Dumb Tool" boundary.
- **Invariant Rule:** **`main.py` accepts only an already-resolved config path.** When running standalone, it uses local `config.example.yaml`. It never crawls parent directories.

---

### Incident 7: Standalone Submodule Path Collision
- **Error Signature:**
  ```text
  FileNotFoundError: .../embedded_system/embedded_system/Tasks not found
  ```
- **Trigger:** Running `cd embedded_system && inv es.ota.release` when `config_tasks.yaml` had `embedded_system_dir: '{workspace_dir}/embedded_system'`.
- **Root Cause:** Standalone execution causes `{workspace_dir}` to evaluate to `embedded_system`, creating double nesting.
- **Invariant Rule:** Subsystem configs must anchor paths using **`{config_dir}`** (e.g. `embedded_system_dir: '{config_dir}'`), guaranteeing identical resolution whether invoked from root or standalone.

---

### Incident 8: Numeric Type Loss via SSoT Token String Expansion
- **Error Signature:**
  ```text
  TypeError: 'str' object cannot be interpreted as an integer
  ValueError: invalid literal for int() with base 10
  ```
- **Trigger:** Reading numeric settings (e.g. `port`, `baud`, `chunk_size_bytes`, `timeout_ms`) from a resolved config in `main.py` without explicit type casting.
- **Root Cause:** The multi-pass string token interpolator in `config_resolver.py` formats values into `{placeholder}` string tokens. If a tool expects native integer types from raw YAML, it receives a string representation instead.
- **Invariant Rule:** **Tier 3 tool engines (`main.py`) must explicitly cast numeric and boolean parameters** using `int(tool_cfg.get("port", default))` or explicit validation schemas. Never assume YAML primitive typing survives token expansion.

---

## 7. Autonomous Self-Healing & Error Recording Protocol

Whenever an AI agent executes, debugs, or modifies host tools and encounters an unexpected runtime error, CLI failure, or regression, it must execute this self-healing loop:

```text
[ Error Encountered ]
       │
       ▼
1. Diagnose Root Cause (Distinguish symptom from architectural cause)
       │
       ▼
2. Implement Minimal Corrective Fix (Code / Task / Config)
       │
       ▼
3. Verify Fix via Verification Checklist (Section 8)
       │
       ▼
4. RECORD IN POST-MORTEM REGISTRY (Section 6):
   - Assign next sequential Incident ID (e.g., Incident 9)
   - Record exact Error Signature
   - Record exact Trigger Condition
   - Record Root Cause
   - Define Permanent Invariant Rule to prevent recurrence
```

### Self-Recording Invariant:
* An AI agent **must not** consider an error resolved until it has permanently appended the lesson into **Section 6 (Post-Mortem & Error Registry)** of this skill file.
* Future AI runs must parse Section 6 prior to proposing or scaffolding tool modifications to prevent repeating known failure patterns.

---

## 8. Verification & Self-Correction Checklist

Before declaring any host tool integration complete, execute this 5-step checklist:

- [ ] **1. Standalone Execution Test:**
  ```bash
  cd embedded_system/Tools/<TOOL_DIR>
  python main.py config.example.yaml
  ```
  *(Must execute successfully with zero dependencies on root project configs or `core`).*

- [ ] **2. Tool Task Isolation Test:**
  ```bash
  inv tools.<tool-slug>.run --config=config.example.yaml
  ```
  *(Must execute via the tool's local `tasks.py`).*

- [ ] **3. Dynamic Auto-Discovery Test:**
  ```bash
  inv --list
  ```
  *(Verify that `tools.<tool-slug>.<task>` appears automatically under the flat `tools` namespace).*

- [ ] **4. SSoT Orchestrator Dry-Run Test:**
  ```bash
  inv es.<tool-name>.run --dry-run
  ```
  *(Verify that SSoT tokens compile into `build/configs/` and delegate cleanly via `inv tools.<tool-slug>.*`).*

- [ ] **5. Zero Path Registrations:**
  *(Confirm that `configs/config_tasks.yaml` does NOT contain any hardcoded `*_script` entries).*

## Error Registry Maintenance

Before host-tool work, review the post-mortem entries in Section 6. Record each verified, reusable mistake discovered during the task there before considering the work complete, including symptom, root cause, and prevention. Do not record speculation, transient output, or secrets.
