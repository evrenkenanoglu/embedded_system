"""
@file       generator.py
@brief      Multi-provider standalone script generator for AI assistants.
"""

import sys
from pathlib import Path
from typing import Dict, List, Type
from src.adapters import BaseAdapter


def detect_platform_format() -> str:
    return "ps1" if sys.platform == "win32" else "sh"


def get_platform_label() -> str:
    if sys.platform == "win32":
        return "Windows (Native)"
    try:
        if "microsoft" in Path("/proc/version").read_text().lower():
            return "Linux (WSL)"
    except Exception:
        pass
    return "Linux/POSIX"


def _build_bash_script(
    adapter: BaseAdapter, env_vars: Dict[str, str], clean_vars: List[str]
) -> str:
    lines = [
        "#!/usr/bin/env bash",
        f"# Auto-generated launcher for [{adapter.name}] using [{adapter.provider_name}]",
        "set -euo pipefail",
        "",
    ]
    if clean_vars:
        lines.append("# Reset conflicting variables")
        for var in clean_vars:
            lines.append(f"unset {var} 2>/dev/null || true")
        lines.append("")

    lines.append(
        f"# Configure [{adapter.name}] environment for [{adapter.provider_name}]"
    )
    for k, v in env_vars.items():
        escaped_v = v.replace('"', '\\"')
        lines.append(f'export {k}="{escaped_v}"')

    binary = adapter.get_binary()
    args_str = " ".join(f'"{a}"' if " " in a else a for a in adapter.get_args())
    exec_line = f'exec {binary} {args_str} "$@"' if args_str else f'exec {binary} "$@"'
    lines.extend(["", "# Execute CLI tool", exec_line, ""])
    return "\n".join(lines)


def _build_powershell_script(
    adapter: BaseAdapter, env_vars: Dict[str, str], clean_vars: List[str]
) -> str:
    lines = [
        f"# Auto-generated launcher for [{adapter.name}] using [{adapter.provider_name}]",
        '$ErrorActionPreference = "Stop"',
        "",
    ]
    if clean_vars:
        lines.append("# Reset conflicting variables")
        for var in clean_vars:
            lines.append(f"Remove-Item Env:\\{var} -ErrorAction SilentlyContinue")
        lines.append("")

    lines.append(
        f"# Configure [{adapter.name}] environment for [{adapter.provider_name}]"
    )
    for k, v in env_vars.items():
        escaped_v = v.replace('"', '`"')
        lines.append(f'$env:{k} = "{escaped_v}"')

    binary = adapter.get_binary()
    args_str = " ".join(f'"{a}"' if " " in a else a for a in adapter.get_args())
    exec_line = f"& {binary} {args_str} @args" if args_str else f"& {binary} @args"
    lines.extend(["", "# Execute CLI tool", exec_line, ""])
    return "\n".join(lines)


def _format_quick_run(project_root: Path, file_path: Path, fmt: str) -> str:
    try:
        rel_path = file_path.relative_to(project_root)
        posix_rel = rel_path.as_posix()
        win_rel = str(rel_path).replace("/", "\\")
    except ValueError:
        posix_rel = file_path.as_posix()
        win_rel = str(file_path).replace("/", "\\")

    if fmt in ("sh", "bash"):
        return f'cd "{project_root}" && "./{posix_rel}"'
    else:
        return f'cd "{project_root}" ; & ".\\{win_rel}"'


def generate_scripts(
    adapter_cls: Type[BaseAdapter],
    config: dict,
    script_format: str,
    output_dir: Path,
    project_root: Path | None = None,
    target_provider: str | None = None,
) -> List[Path]:
    """Generates run_<tool>_<provider>.(sh|ps1) for each defined provider."""
    output_dir.mkdir(parents=True, exist_ok=True)
    root = project_root or Path.cwd()
    ai_cfg = config.get("ai_assistant", config)
    providers = ai_cfg.get("providers", {})

    # Determine which providers to generate
    if target_provider:
        provider_names = [target_provider]
    elif providers:
        provider_names = list(providers.keys())
    else:
        provider_names = ["default"]

    # Determine target file extensions
    fmt_key = script_format.lower()
    if fmt_key == "auto":
        detected_fmt = detect_platform_format()
        print(
            f"[AI Assistant] Auto-detected OS: {get_platform_label()} -> Target: .{detected_fmt}"
        )
        formats = [detected_fmt]
    elif fmt_key in ("both", "all"):
        formats = ["sh", "ps1"]
    else:
        formats = [fmt_key]

    generated_files = []
    print(
        f"\n[AI Assistant] Generating scripts across {len(provider_names)} provider(s)..."
    )

    for p_name in provider_names:
        # Create an adapter instance pinned to this provider
        adapter = adapter_cls(config, provider_override=p_name)
        configured_env = adapter.configure_env({})
        clean_vars = adapter.tool_config.get("clean_env", [])

        # File naming: run_<tool>_<provider>.<ext>
        base_filename = f"run_{adapter.name}_{p_name}"

        for fmt in formats:
            if fmt in ("sh", "bash"):
                file_path = output_dir / f"{base_filename}.sh"
                content = _build_bash_script(adapter, configured_env, clean_vars)
                file_path.write_text(content, encoding="utf-8")
                if sys.platform != "win32":
                    file_path.chmod(0o755)
                generated_files.append(file_path)

                quick_cmd = _format_quick_run(root, file_path, "sh")
                print(f"  [+] {file_path.name}")
                print(f"      \033[1;32m{quick_cmd}\033[0m")

            elif fmt in ("ps1", "powershell"):
                file_path = output_dir / f"{base_filename}.ps1"
                content = _build_powershell_script(adapter, configured_env, clean_vars)
                file_path.write_text(content, encoding="utf-8")
                generated_files.append(file_path)

                quick_cmd = _format_quick_run(root, file_path, "ps1")
                print(f"  [+] {file_path.name}")
                print(f"      \033[1;36m{quick_cmd}\033[0m")

    print("")
    return generated_files
