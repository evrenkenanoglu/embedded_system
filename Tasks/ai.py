import sys

from invoke import Context, task


@task(
    help={
        "dry_run": "Print command line without executing",
        "opts": "Forward additional arbitrary arguments",
    }
)
def serialize(c: Context, dry_run: bool = False, opts: str = "") -> None:
    """Run the AI Content Serializer pipeline (manifest + context serialization)."""
    c.run(
        f'"{sys.executable}" -m invoke tools.ai-content-serializer.serialize',
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )
