---
name: ci-cd-pipeline
description: >-
  Design, modify, troubleshoot, and validate the repository's declarative CI and
  HIL pipelines, Docker execution, credential mounts, OTA artifacts, and pipeline engine.
---

# CI/CD Pipeline Skill

Use this skill when changing or debugging repository-root `CI_CD/` configuration or the pipeline engine under `embedded_system/CI_CD/`.

## Architecture and sources of truth

- `configs/config_project.yaml` is the project SSoT; `CI_CD/config.yaml` provides common CI settings, and `CI_CD/config_ci.yaml` / `CI_CD/config_hil.yaml` define pipeline stages.
- `embedded_system/Tasks/ci_cd.py` resolves configuration and invokes `PipelineEngine`.
- `embedded_system/CI_CD/pipeline/core/engine.py` handles sequential host, Docker, and artifact stages. `docker_manager.py`, `git_manager.py`, and `utils.py` implement those boundaries.
- Preserve platform independence (Linux, WSL, Windows). Use path tokens and the pipeline path conversion helpers rather than hard-coded host paths.

## Pipeline stage contracts

- `host`: filesystem actions and host commands; source checkout may require Git credentials and initialized submodules.
- `docker`: toolchain image, explicit working directory, environment setup, declared pip dependencies, and optional volume mounts.
- `artifact`: bundle/extract only the declared output paths. Keep package destinations outside source traversal to avoid recursive packaging.
- Keep stage order and responsibilities explicit. CI builds/packages firmware; HIL extracts, flashes, and tests it on physical hardware.

## OTA and credential handling

- Use the canonical OTA release task (`inv es.ota.release`) rather than reimplementing build, signing, catalog reconciliation, or packaging steps in YAML.
- Signing keys and certificates are secret inputs. Mount `{paths.keys_dir}` and `{paths.certs_dir}` read-only into the checkout; do not commit them, print their contents, or include them in artifacts.
- The OTA release catalog is persistent state used for manifest history and delta generation. Mount it at its canonical path with write access only where required; package only the manifest, release binaries, and patches intended for downstream use.
- Ensure the requested `target`, channel/policy, version metadata, and release archive name come from the SSoT/task contract; do not silently fork policy in the pipeline YAML.

## Docker and workspace safety

- `DockerManager` mounts the project at `/project`, normalizes destinations beneath it, enforces the requested workdir, and may mount the host pip cache.
- Docker runs that write into a bind-mounted workspace can create root-owned build files. Prefer isolated CI workspaces and non-root execution where compatible. After local root-owned output is created, repair or clean only known generated directories using the cleanup abstraction; never blanket-`chown` credentials or the entire repository.
- Before deleting or cleaning paths, verify they resolve to generated output locations. Preserve source trees and user changes.
- A stale `build/CMakeCache.txt` created from another host/container path can cause CMake source/build mismatch. Clean that generated build directory using the supported cleanup path before retrying; do not assume a read-only key mount addresses this separate issue.

## Validation procedure

1. Inspect changes and relevant configurations; verify required host mounts exist without reading or logging secret material.
2. Resolve and validate the pipeline configuration with `inv es.ci-cd.ci --dry-run` (and `inv es.ci-cd.hil --dry-run` when HIL config is affected).
3. Test the narrowest affected engine/mount behavior and run relevant Python tests or compilation plus `git diff --check`.
4. Run full CI only when Docker, image access, Git checkout credentials, and required signing inputs are available. State clearly if testing stopped before build or release stages.
5. Never execute HIL flashing without explicit authorization and confirmed hardware readiness. A dry run is not evidence of a successful build, OTA signing, artifact package, or hardware test.

## Known failure patterns

| Symptom | Likely cause | Response |
| --- | --- | --- |
| `Secure Boot Signing Key ... does not exist` inside container | Ignored host `keys/` was not mounted into checkout | Verify the read-only `{paths.keys_dir}` mount maps to `<checkout>/keys`; confirm key presence without printing its contents. |
| Formatter/config resolver cannot write under `build/configs/` | Local Docker build left the host `build/` tree root-owned | Restore access only to generated build outputs; verify by running the relevant task with `--dry-run`. |
| CMake cache source path differs from `/project` | Reused build cache was generated under a different host path | Clean the generated build directory through `DockerManager.cleanup_path`, then rebuild. |
| Checkout fails with GitHub username/auth prompt | Pipeline's source-control stage cannot authenticate to the configured remote | Configure the approved Git credential mechanism; do not embed tokens in YAML or shell commands. |
| Build succeeds but OTA history/deltas disappear | Release catalog was not persisted between clean checkouts | Bind the canonical catalog path across runs and ensure it is writable by the release process. |
| HIL tests cannot access a device | Wrong port, missing permissions, or hardware unavailable | Resolve the configured platform port and obtain hardware readiness before running flash/test stages. |
