import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("Error: PyYAML missing. Run: pip install pyyaml")
    sys.exit(1)

from src.adapters import ADAPTERS
from src.generator import generate_scripts


def main():
    parser = argparse.ArgumentParser(
        description="AI Assistant Interactive Runner & Script Generator"
    )
    parser.add_argument(
        "config",
        type=Path,
        help="Path to the YAML configuration file (resolved by task runner)",
    )
    parser.add_argument(
        "-t",
        "--tool",
        choices=list(ADAPTERS.keys()),
        help="Override active tool (copilot, claude, antigravity, generic)",
    )
    parser.add_argument(
        "-p",
        "--provider",
        help="Target provider backend (e.g. openrouter, ollama, groq). Defaults to all when generating.",
    )
    parser.add_argument(
        "--generate",
        nargs="?",
        const="auto",
        default=None,
        choices=["auto", "sh", "bash", "ps1", "powershell", "both"],
        help="Generate standalone launch scripts (default: auto-detect OS)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="Target folder for generated scripts (defaults to current directory/scripts)",
    )

    args, extra_args = parser.parse_known_args()

    # 1. Validate Config File Input
    config_file = args.config.resolve()
    if not config_file.is_file():
        print(f"Error: Configuration file '{config_file}' not found.")
        sys.exit(1)

    with open(config_file, "r", encoding="utf-8") as f:
        config = yaml.safe_load(f) or {}

    ai_cfg = config.get("ai_assistant", config)

    # 2. Select Adapter Class
    tool_name = args.tool or ai_cfg.get("active_tool", "copilot")
    adapter_cls = ADAPTERS.get(tool_name.lower())
    if not adapter_cls:
        print(f"Error: Unknown tool '{tool_name}'. Available: {list(ADAPTERS.keys())}")
        sys.exit(1)

    # 3. Handle Script Generation
    if args.generate:
        target_dir = args.output_dir or (Path.cwd() / "scripts")
        generate_scripts(
            adapter_cls=adapter_cls,
            config=config,
            script_format=args.generate,
            output_dir=target_dir,
            project_root=Path.cwd(),
            target_provider=args.provider,
        )
        sys.exit(0)

    # 4. Standard Interactive Run
    adapter = adapter_cls(config, provider_override=args.provider)

    binary_name = adapter.get_binary()
    binary_path = shutil.which(binary_name)
    if not binary_path:
        print(f"Error: '{binary_name}' CLI is not installed or not in PATH.")
        sys.exit(127)

    env = os.environ.copy()
    env = adapter.configure_env(env)

    cmd = [binary_path] + adapter.get_args()
    if extra_args:
        cmd.extend(extra_args)

    use_shell = sys.platform == "win32" and binary_path.lower().endswith(
        (".cmd", ".bat")
    )

    print(f"[AI Assistant] Config   : {config_file}")
    print(f"[AI Assistant] Tool     : {tool_name}")
    print(
        f"[AI Assistant] Provider : {adapter.provider_name} ({adapter.provider.get('model', 'default')})"
    )
    print(f"[AI Assistant] Command  : {' '.join(cmd)}\n")

    if sys.platform != "win32":
        os.execvpe(binary_path, cmd, env)
    else:
        try:
            res = subprocess.run(cmd, env=env, shell=use_shell)
            sys.exit(res.returncode)
        except KeyboardInterrupt:
            print("\n[AI Assistant] Interactive session terminated.")
            sys.exit(0)


if __name__ == "__main__":
    main()
