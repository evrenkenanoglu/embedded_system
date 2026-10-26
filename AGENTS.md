# Embedded System Agent Instructions

These instructions supplement the repository-root `AGENTS.md` and apply to `embedded_system/`.

## Embedded architecture

- Read the applicable skill under `embedded_system/.agents/skills/` before changing firmware, OTA, provisioning, security, utilities, or hardware-testing behavior.
- Keep `embedded_system/Tasks/` Invoke modules thin: resolve configuration from the shared SSoT, then delegate to the toolchain or standalone tool. Do not bypass canonical tasks with ad hoc shell commands.
- Keep standalone tool engines independent of the task-runner `core`; pass them resolved configuration and avoid crawling parent directories or duplicating SSoT logic.
- Follow the toolchain boundary: common lifecycle behavior belongs in `BaseToolchain`; vendor-specific operations belong in the ESP-IDF adapter.
- Treat generated firmware configuration and OTA release catalog data as generated artifacts. Update their canonical source/configuration and generator instead of hand-editing derived files.

## CI/CD pipeline

- For CI/CD changes, load `.agents/skills/ci-cd-pipeline/SKILL.md` as the canonical pipeline guidance.
- The declarative pipeline configuration lives in the repository-root `CI_CD/`; the engine and Docker abstraction live in `embedded_system/CI_CD/pipeline/`.
- Preserve stage separation between host operations, Docker execution, and artifact handling. Resolve tokens before execution; keep path and mount handling platform-independent.
- Mount signing keys/certificates read-only and keep OTA release history in its designated persistent catalog. Never place secret contents in logs or release archives.
- Do not run the HIL flash/test pipeline without explicit authorization and confirmed target hardware/port.
- For CI changes, validate configuration resolution and dry runs, then run the narrowest applicable tests. Document any inability to run end-to-end tests (for example missing Git credentials, Docker, credentials, or hardware).

## Workspace hygiene

- Inspect `git status` in both the parent repository and this nested project before changes. Preserve user-owned modifications.
- Avoid leaving Docker-created root-owned files in developer output directories; use the pipeline cleanup abstraction and limit ownership repair to generated build/output directories.
- Do not commit unless requested.
