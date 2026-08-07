# LuminariMUD Conventions

This document is the canonical repository convention reference. `AGENTS.md` remains authoritative
for agent-specific rules; this page records the durable engineering, build, test, documentation,
and operational conventions that apply to contributors.

## Authority and Scope

- Trace declarations, storage, callers, and writers before editing; do not infer contracts from
  names or nearby patterns.
- Target only the supported Luminari build. Treat legacy campaign code as compatibility inventory.
- Work only in development checkouts. Never modify production code or create a branch or worktree
  from production.
- Preserve unrelated user changes in a dirty worktree.
- Prefer the smallest behavior-preserving change that satisfies the current objective.

## GNU C23 Style

- Use GNU C23 while matching established CircleMUD/tbaMUD source style. Do not mechanically
  restyle legacy code.
- Indent two spaces, use Allman braces, keep code to 100 columns, and right-align pointers.
- Use `lower_snake_case` for functions and variables, `UPPER_SNAKE_CASE` for constants and macros,
  and `*_data` names for structures.
- Use `/* */` comments, explain why rather than restating code, and keep declarations at block
  tops.
- Do not use variable-length arrays. Use `snprintf`, never `sprintf`, and NULL-check before
  dereferencing.
- Log actionable runtime failures with `log("SYSERR: ...")`; fix all `-Wall -Wextra` warnings.
- Use repository macros and typed VNUM definitions after tracing them; never hard-code virtual
  numbers.

Build configuration prefers `-std=gnu23`. It accepts the legacy `-std=gnu2x` spelling only when the
compiler passes the required C23 keyword probe.

## Includes and Interfaces

- Preserve the common include order: `conf.h`, `sysdep.h`, `structs.h`, then `utils.h`.
- Headers at `src/` are included by bare name. Headers in a feature directory are path-qualified
  outside that directory, such as `#include "spec/spec_registry.h"`.
- Do not add per-directory include flags to hide cross-subsystem dependencies.
- Keep public APIs narrow. Place private declarations in an internal header rather than exporting
  them solely to support a file split.

## Files and Ownership

- `src/` uses one flat level of feature directories; do not introduce second-level nesting.
- File membership follows primary responsibility, not every subsystem a function touches.
- Keep cohesive feature or zone content together when it shares state, sequencing, VNUMs, and
  private helpers.
- Leave shops, quests, vessels, crafting, magic, artifacts, and other mature systems with their
  owning subsystem.
- When adding or removing any source or CuTest file, update both `Makefile.am` and
  `CMakeLists.txt`.

## Memory and Error Safety

- Use the repository allocation and lifecycle helpers after tracing their ownership contracts.
- Allocate replacement state completely before releasing the old state when an operation must be
  transactional.
- Free every owned allocation on normal and failure paths. Use Valgrind and sanitizers to verify
  affected lifecycles.
- Validate pointer identity and array bounds before access. A matching VNUM does not prove that two
  object instances are the same object.
- Never inspect an owner, actor, or target after a callback that may have extracted it unless the
  caller has an explicit lifetime guarantee.
- Bound strings, file reads, diagnostics, and cleanup targets. Never silently truncate data whose
  exact identity is part of a persistence or security contract.

## Special Procedure Refactor

- Preserve the legacy `SPECIAL` ABI, canonical persisted names, world formats, dispatch order,
  scheduling, return behavior, activation flags, and boot precedence until tests authorize a
  change.
- Keep procedure definitions, authored binding state, effective provenance, and invocation
  gateways as separate responsibilities.
- Construct typed event context at call sites while complete data is available; translate legacy
  arguments only at the compatibility seam.
- Treat callback pointers as borrowed for one synchronous invocation. Cache iteration successors
  before handlers that may extract owners or targets.
- World data is the preferred authoring source for new reusable bindings, but compatibility
  precedence remains authoritative until deliberately migrated.
- Preserve unresolved authored names or block their implicit OLC overwrite; a diagnostic alone
  does not prevent data loss.
- Add shared helpers only for a named game rule with at least two audited consumers and focused
  tests.
- Keep artifact ownership, custody, progression, and persistence out of the general special
  procedure subsystem.
- Prefer DG Scripts for localized narrative, dialogue, puzzles, and sequencing without
  engine-level lifecycle requirements.
- Treat quest-over-shop-over-original dispatch as explicit runtime compatibility composition, not
  as a persisted prototype chain. Do not generalize it without an approved second consumer and a
  versioned OLC/persistence contract.
- Add engine lifecycle behavior first as a direct typed call at its owning subsystem. Generalize
  only after multiple consumers prove identical ordering, veto, re-entry, and lifetime rules.

The enduring architecture and migration lessons are in
[Project Considerations](CONSIDERATIONS.md#special-procedure-architecture-refactor). Current and
completed scope and conditional reopen criteria are in the
[Special Procedure Refactor PRD](ongoing-projects/spec-todo.md).

## Local Configuration and Credentials

- Never modify `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`; edit the matching
  `.example.h` template only when a shared template change is required.
- Never modify credential-bearing `lib/.env` or `lib/mysql_config` without explicit permission;
  edit `lib/.env_example` or `lib/mysql_config_example` instead.
- Copy an example only on a fresh clone when the real local file does not exist. Never overwrite a
  configured local file.
- Never expose credentials in logs, test output, documentation, commits, or diagnostics.

## Database Layer

- MariaDB/MySQL is required. Access it through the established `src/mysql.c` integration and C
  client dependency.
- Keep connection details in the existing local credential files and environment, never in source.
- Escape or parameterize untrusted data according to existing database helpers; never concatenate
  raw player input into SQL.
- Keep schema and setup changes under the existing `sql/` and deployment conventions, with tests,
  verification, rollback artifacts, and documentation appropriate to the affected subsystem.
- Free result sets and other client-library resources on every exit path.

## Testing

- Use root production-linked CuTest coverage for vessel, vehicle, autopilot, special-procedure, and
  other behavior that touches real game sources and structures.
- Run `make test`, then always run `make install`; do not leave a root-level `circle` artifact.
- There is no `test_runner` binary. The focused protocol harness is
  `unittests/CuTest/protocol_parser_tests` and is not a substitute for production-linked coverage.
- Add `unittests/CuTest/test_*.c` files to `cutest_SOURCES` and `cutest_test_files` in
  `Makefile.am`, and to `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.
- Characterize observable legacy behavior before refactoring it. Test behavior and safety
  contracts, not private implementation layout.
- Cover normal and `-s` modes, OLC round trips, boot precedence, aliases, invalid metadata,
  extraction, iteration safety, and exact event translation where applicable.
- CuTest has executable-level granularity; there is no per-test-function filter.

## Build and Tooling

- Prefer incremental Autotools builds: `make clean`, `make -j"$(nproc)"`, then `make install`.
- If generated configuration is absent, run `autoreconf -fvi` and `./configure` first.
- Use CMake as a supported secondary build and keep its manifests behaviorally synchronized. Fresh
  CMake test trees require `-DBUILD_TESTS=ON`.
- Use `.clang-format` for formatting and `.clang-tidy` for configured static analysis.
- Respect the pre-commit hooks, including include-comment alignment changes. Rebuild and retest
  after formatting modifies source.
- The configured pre-commit package lives in `.venv`; run it with
  `.venv/bin/pre-commit run --all-files` when a full manual hook pass is required.

## Documentation

- Update relevant builder guides, helpfiles, architecture, developer, system, testing, and master
  index documentation when behavior changes.
- Document implemented behavior as current and leave later phases clearly labeled as proposals.
- Keep one source of truth for each contract and link to it instead of duplicating explanations.
- Keep all documentation valid ASCII, UTF-8, and Unix LF. Use root-relative context only when a
  command explicitly runs from the repository root; use valid relative Markdown links elsewhere.
- Store feature product contracts in `docs/product-requirements/` when they are durable release
  contracts. Active plans live under `docs/ongoing-projects/` until their enduring content is
  consolidated.

## Git and Review

- Never attribute AI or assistants in commits or project artifacts.
- Use concise imperative commit messages and keep each commit to one logical, independently
  testable change.
- Keep commits atomic enough to review and revert safely.
- Review changes against the relevant base commit and preserve existing authored content and
  history.
- Do not rewrite historical paths in `docs/CHANGELOG.md` or `docs/previous_changelogs/`; those files
  record the tree as it existed.

## Local Development Tools

| Category | Tool | Configuration or entry point |
|----------|------|------------------------------|
| Compiler | GNU-compatible C23 compiler | `configure.ac`, `CMakeLists.txt` |
| Formatter | clang-format | `.clang-format` |
| Linter/static analysis | clang-tidy | `.clang-tidy` |
| Type safety | GCC/Clang `-Wall -Wextra` | `Makefile.am`, `CMakeLists.txt` |
| Testing | CuTest and protocol parser harness | `Makefile.am`, `unittests/CuTest/Makefile` |
| Build | Autotools/Automake and CMake | `Makefile.am`, `CMakeLists.txt` |
| Development startup | Syntax-check boot | `./bin/circle -c -d lib` |
| Observability | Autorun structured crash capture | `scripts/autorun/autorun.sh`, `log/last_error_*.json` |
| Git hooks | pre-commit | `.pre-commit-config.yaml` |
| Database | MariaDB/MySQL C client | `src/mysql.c`, `sql/`, `lib/mysql_config` |

Autorun writes abnormal-exit context atomically as mode `0600` JSON under the existing ignored
`log/` runtime directory. The record includes immutable release identity and exact core/backtrace
paths without copying configuration or credential values.

## CI/CD

| Bundle | Workflow or configuration | Enforced contract |
|--------|---------------------------|-------------------|
| Quality | `.github/workflows/quality.yml` | clang-format, clang-tidy, and warning-clean build |
| Tests | `.github/workflows/test.yml` | Production-linked CuTest, world tools, sanitizers, Valgrind, MariaDB, and coverage |
| Security | `.github/workflows/security.yml` | Secret scanning, CodeQL, and dependency review |
| Integration | `.github/workflows/integration.yml` | Schema dry-run, minimal-world validation, and network startup smoke test |
| Operations | `.github/workflows/release.yml`, `.github/workflows/pages.yml`, `.github/dependabot.yml` | Release artifacts, documentation publishing, and dependency updates |

CI test boots use `scripts/ci/prepare_test_runtime.sh` with a local isolated MariaDB service and
`.ci-runtime/lib`. The preparer refuses the protected repository `lib/` tree and non-local database
hosts.

## Infrastructure

| Component | Provider or entry point | Contract |
|-----------|-------------------------|----------|
| Hosting | Self-managed systemd | `luminari.service` supervises `scripts/autorun/autorun.sh` |
| Database | MariaDB/MySQL | Required runtime dependency; credentials stay in `lib/mysql_config` |
| Health | Luminari loopback HTTP | `/health` checks game-loop and MariaDB readiness on port 8182 |
| Startup probe | systemd `ExecStartPost` | Bounded `scripts/operations/healthcheck.sh --wait` check |
| Process diagnosis | Copyover watchdog | `scripts/copyover/copyover_watchdog.sh` |
| Backup | Repository backup script | `scripts/backup.sh` |
| World backup | Zone backup helper | `lib/world/backup-zone.sh` |
| Deployment | Automated setup and service install | `scripts/deployment/deploy.sh` |

The health listener is loopback-only. `TERRAIN_API_PORT` changes its port and
`LUMINARI_HEALTH_URL` must identify the matching readiness URL. The CI smoke test uses port 4182
against an isolated MariaDB-backed runtime.

## When In Doubt

- Decide from traced repository evidence and document material assumptions.
- Preserve observable behavior until an intentional change has its own specification and tests.
- Rebuild, test, install, and update documentation before declaring implementation complete.
