---
name: embedded-utility-tool-architect
description: Authoritative specification and code-generation standard for designing, implementing, and integrating host-side utility tools into the Embedded_IoT_BT_WIFI_Base_Project ecosystem. Enforces strict decoupling between dumb CLI engines, standalone tool tasks, and SSoT domain orchestrator tasks.
version: 4.0.0
target_runtime: Python 3.13 / uv / invoke
target_project: Embedded_IoT_BT_WIFI_Base_Project
triggers:
  - "create a host tool"
  - "implement utility tool"
  - "add new tool to embedded_system/Tools"
  - "scaffold tool"
---

# Skill: Host Utility Tool Architect & Task Automation

## 1. Core Architectural Mental Model (The 3-Tier Decoupled Flow)

Every tool in this ecosystem strictly separates responsibilities into 3 distinct tiers:

```
┌────────────────────────────────────────────────────────────────────────┐
│ TIER 1: SSoT Domain Task (embedded_system/Tasks/<tool_name>.py)        │
│  - System-level orchestrator mounted under `inv es.<tool_name>.<task>` │
│  - Imports `core.config_resolver.resolve_to_file` and `core.CONFIG`.   │
│  - Compiles SSoT tokens into `build/configs/<tool>.resolved.yaml`.     │
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

## 2. Invariants & Strict Guardrails

### ❌ Absolute Prohibitions (Anti-Patterns)
1. **NO Hardcoded Script Paths in YAML**: Never register `<tool>_script` or `<tool>_runner_script` in `config_tasks.yaml`. All tools are discovered dynamically by scanning `**/tasks.py`.
2. **NO Direct Script Execution from `Tasks/`**: `embedded_system/Tasks/<tool>.py` must never execute `python <path>/main.py`. It resolves SSoT tokens and delegates via `inv tools.<tool>.<task>`.
3. **NO SSoT Imports in `Tools/`**: Never import `core`, `CONFIG`, or `core.config_resolver` inside `embedded_system/Tools/<TOOL_NAME>/` or its `tasks.py`.
4. **NO Reserved Invoke Parameter Names**: Never name an Invoke task argument `config` (collides with Invoke's global `--config` flag). Always use `config_path`.
5. **NO Hardcoded Python Strings**: Never use `f'python "{script}"'`. Always use `f'"{sys.executable}" "{script}"'` to guarantee virtual environment execution on POSIX/WSL.
6. **NO Directory Traversal**: `main.py` must never crawl parent directories (`parents[3]`, `.git`). It reads *only* the config path passed to it.
7. **NO Greedy CLI Parsing**: Never use `nargs=argparse.REMAINDER`. Always use `parser.parse_known_args()`.

### ✅ Mandatory Requirements
1. **Colocated `tasks.py`**: Every tool in `embedded_system/Tools/<TOOL>/` must have its own `tasks.py` that mirrors `main.py`.
2. **Flat Tool Namespacing**: Tools are mounted directly under `tools.<tool-slug>.*`. Intermediate folders on disk (`AI/`, `HW_SECURITY/`) are ignored to keep CLI commands stable.
3. **Interactive vs. Batch Process Separation**:
   - For interactive tools (TUI, REPL, Chat), pass `pty=(sys.platform != 'win32')` in Invoke, and prefer `os.execvpe` on POSIX in `main.py`.
4. **Dependency Pinning**:
   - Unpinned dependencies in `requirements.in`.
   - Lockfile `requirements.txt` compiled via `uv pip compile requirements.in -o requirements.txt`.

---

## 3. Canonical File Tree Specification

```text
Embedded_IoT_BT_WIFI_Base_Project/
├── configs/
│   ├── config_tasks.yaml                       # Root workspace settings only (NO tool scripts)
│   └── config_<tool_name>.yaml                 # SSoT domain configuration
├── embedded_system/
│   ├── config_tasks.yaml                       # Subsystem tasks & discovery rules (anchored to {config_dir})
│   ├── Tasks/
│   │   └── <tool_name>.py                      # Tier 1: SSoT Orchestrator task (resolves tokens, calls inv)
│   └── Tools/
│       └── <TOOL_NAME>/                        # Tier 2 & 3: Standalone Tool Directory
│           ├── src/
│           │   ├── __init__.py
│           │   └── ...                         # Internal logic / adapters
│           ├── config.example.yaml             # Standalone local config (NO tokens, NO base_config)
│           ├── main.py                         # Tier 3: Dumb CLI entry point
│           ├── tasks.py                        # Tier 2: Colocated basic Invoke task wrapper
│           ├── requirements.in                 # Unpinned dependencies
│           ├── requirements.txt                # Lockfile compiled via uv
│           └── README.md                       # CLI usage & documentation
```

---

## 4. Implementation Templates

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
    print(f"[<TOOL_NAME>] Active Config: {config_file}")

    # Core engine logic here
    sys.exit(0)


if __name__ == "__main__":
    main()
```

---

### Template 2: Tier 2 — Colocated Standalone Task (`Tools/<TOOL>/tasks.py`)
*Zero `core` imports. Mirrors `main.py`. Uses `sys.executable`.*

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

    # 1. Resolve SSoT config here
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

## 5. Post-Mortem & Error Registry (Crucial for AI Self-Correction)

Consult this failure registry before writing or debugging any task code:

### Incident 1: Invoke Reserved `--config` Argument Collision
- **Error:** `invoke.exceptions.UnknownFileType: Config files of type '' (from file '') are not supported!`
- **Trigger:** Naming an Invoke task parameter `config: str = ""` and running `inv <task> --config=`.
- **Root Cause:** `--config` is a reserved global flag in Invoke itself. Passing an empty string causes Invoke to try loading `""` as an internal config file before the task runs.
- **Rule:** **NEVER name an Invoke task parameter `config`. Always use `config_path` or `cfg`.**

### Incident 2: Path-Dependent Tool Task Disappearance
- **Error:** `No idea what 'tools.<tool-name>.run' is!`
- **Trigger:** Reorganizing a tool on disk (e.g. moving to `Tools/AI/Tool/`) changes the task name to `tools.ai.tool.run`.
- **Root Cause:** Discoverer used the full relative path instead of anchoring to the tool's folder name.
- **Rule:** Tool discovery must be **flat under `tools`**. The command is always `tools.<tool-slug>.<task>`, completely decoupled from disk depth.

### Incident 3: Missing System Binary on POSIX / WSL
- **Error:** `/bin/bash: line 1: python: command not found (exit code 127)`
- **Trigger:** Calling `c.run('python main.py ...')` on Ubuntu/WSL.
- **Root Cause:** Modern Linux systems ship with `python3`. `python` does not exist in standard subshells.
- **Rule:** **Always use `sys.executable`** (`f'"{sys.executable}" "{script}"'`).

### Incident 4: Subprocess Dropped Enter Keystroke on POSIX
- **Error:** Interactive CLI/TUI tools (chat, serial monitor) drop the `Enter` key on WSL/Linux.
- **Root Cause:** Invoke defaults to `pty=False`. Node.js/React Ink TUIs require raw mode PTY (`\r` vs `\n`).
- **Rule:** Always pass `pty=(sys.platform != "win32")` when running interactive tools.

### Incident 5: CLI Argument Swallowing via `REMAINDER`
- **Error:** `error: unexpected argument '--tool' found`
- **Root Cause:** `nargs=argparse.REMAINDER` greedily consumes subsequent options intended for `main.py` and forwards them to child tools.
- **Rule:** **Always use `parser.parse_known_args()`.**

---

## 6. Verification Checklist

- [ ] **1. Standalone Execution:** `cd embedded_system/Tools/<TOOL> && python main.py config.example.yaml` succeeds without SSoT dependencies.
- [ ] **2. Tool Task Isolation:** `inv tools.<tool-slug>.run --config=config.example.yaml` executes via the tool's local `tasks.py`.
- [ ] **3. Dynamic Auto-Discovery:** `inv --list` shows `tools.<tool-slug>.<task>` under the flat `tools` namespace.
- [ ] **4. SSoT Orchestration:** `inv es.<tool-name>.run --dry-run` compiles tokens into `build/configs/` and delegates to `inv tools.<tool-slug>.run`.
- [ ] **5. Zero Script Registrations:** Confirmed that `configs/config_tasks.yaml` does **not** contain any hardcoded `*_script` entries.
