# Recommended Code Convention Files

**This directory no longer holds rule text.** The single source of truth
for all C++ code conventions is the skills system:

- **General C++ rules** (naming, formatting, error handling, RAII,
  logging, concurrency, Doxygen):
  `embedded_system/.agents/skills/embedded-cpp-code-conventions/SKILL.md`

- **HAL structure and driver naming** (directory layout,
  `io_`/`com_`/`cpx_`/`mem_`/`sys_` taxonomy, interface vs
  implementation, known deviations):
  `embedded_system/.agents/skills/embedded-hal-architecture/SKILL.md`

The former per-topic files in this directory are link stubs kept only
for backwards compatibility with tooling that scans this folder.
