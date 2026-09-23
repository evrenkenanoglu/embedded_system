from pathlib import Path
from typing import Literal
from invoke import Context, task
from core import CONFIG, CommandSerializer
from core.config_resolver import resolve_to_file

FormatLang = Literal["all", "c", "python", "yaml", "cmake"]
FormatMode = Literal["apply", "check"]
FormatScope = Literal["changed", "all"]

VALID_FORMAT_LANGS = ("all", "c", "python", "yaml", "cmake")
VALID_FORMAT_MODES = ("apply", "check")
VALID_FORMAT_SCOPES = ("changed", "all")


@task(
    help={
        "dry_run": "Print execution command without running",
        "opts": "Forward arbitrary arguments to the system file generator script",
    }
)
def system_file_generator(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Run the System Source Files Generator script."""
    script_path = CONFIG.paths.system_file_generator_script
    if not script_path.exists():
        raise FileNotFoundError(
            f"System file generator script not found: {script_path}"
        )

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )
    serializer.add(f'python "{script_path}"', extra=opts)
    serializer.run(c, dry_run=dry_run)


def _format_code(
    c: Context,
    mode: FormatMode = "apply",
    lang: FormatLang = "all",
    scope: FormatScope = "changed",
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Internal helper to resolve SSoT config to a flat temporary file and invoke code formatter."""
    if mode not in VALID_FORMAT_MODES:
        raise ValueError(
            f"Invalid mode '{mode}'. Available options: {', '.join(VALID_FORMAT_MODES)}"
        )

    if lang not in VALID_FORMAT_LANGS:
        raise ValueError(
            f"Invalid language '{lang}'. Available options: {', '.join(VALID_FORMAT_LANGS)}"
        )

    if scope not in VALID_FORMAT_SCOPES:
        raise ValueError(
            f"Invalid scope '{scope}'. Available options: {', '.join(VALID_FORMAT_SCOPES)}"
        )

    script_path = Path(CONFIG.paths.code_formatter_script).resolve()
    if not script_path.exists():
        raise FileNotFoundError(f"Code formatter script not found: {script_path}")

    # 1. Identify input template config
    raw_config = config or getattr(CONFIG.paths, "es_config_formatter", "")
    if not raw_config:
        raise ValueError("No formatter configuration file specified.")

    # 2. Compile into a flat, standalone YAML file in build/configs/
    resolved_config_file = resolve_to_file(
        raw_config, workspace_root=CONFIG.paths.workspace_dir
    )

    serializer = CommandSerializer(
        prefix_commands=[CONFIG.venv_activate_cmd],
        env=CONFIG.env,
    )

    cmd = (
        f'python "{script_path}" '
        f'--config "{resolved_config_file}" '
        f"--mode {mode} "
        f"--lang {lang} "
        f"--scope {scope}"
    )
    serializer.add(cmd, extra=opts)
    serializer.run(c, dry_run=dry_run)


@task(
    help={
        "lang": f"Target language: {', '.join(VALID_FORMAT_LANGS)} (default: 'all')",
        "scope": f"File scope: {', '.join(VALID_FORMAT_SCOPES)} (default: 'changed')",
        "dry_run": "Print command line without executing",
        "config": "Path to custom formatter configuration file",
        "opts": "Forward additional arbitrary arguments",
    }
)
def format_apply(
    c: Context,
    lang: FormatLang = "all",
    scope: FormatScope = "changed",
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Apply formatting in-place (defaults to only git-modified/uncommitted files)."""
    _format_code(
        c,
        mode="apply",
        lang=lang,
        scope=scope,
        dry_run=dry_run,
        config=config,
        opts=opts,
    )


@task(
    help={
        "lang": f"Target language: {', '.join(VALID_FORMAT_LANGS)} (default: 'all')",
        "scope": f"File scope: {', '.join(VALID_FORMAT_SCOPES)} (default: 'all')",
        "dry_run": "Print command line without executing",
        "config": "Path to custom formatter configuration file",
        "opts": "Forward additional arbitrary arguments",
    }
)
def format_check(
    c: Context,
    lang: FormatLang = "all",
    scope: FormatScope = "all",
    dry_run: bool = False,
    config: str = "",
    opts: str = "",
) -> None:
    """Check formatting across the workspace without modifying files (CI/CD dry-run)."""
    _format_code(
        c,
        mode="check",
        lang=lang,
        scope=scope,
        dry_run=dry_run,
        config=config,
        opts=opts,
    )
