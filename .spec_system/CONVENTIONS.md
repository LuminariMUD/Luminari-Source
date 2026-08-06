# CONVENTIONS.md

## Authority and Scope

- Follow the repository `AGENTS.md`; trace declarations, storage, callers, and writers before edits.
- Target only the supported Luminari build. Treat legacy campaign code as compatibility inventory.
- Work only in development checkouts. Never modify production code or create branches from
  production.
- Preserve unrelated user changes in a dirty worktree.

## GNU C23 Style

- Use GNU C23 while matching established CircleMUD/tbaMUD source style; do not mechanically restyle
  legacy code.
- Indent two spaces, use Allman braces, keep code to 100 columns, and right-align pointers.
- Use `lower_snake_case` for functions and variables, `UPPER_SNAKE_CASE` for constants and macros,
  and `*_data` names for structures.
- Use `/* */` comments, explain why rather than restating code, and keep declarations at block tops.
- Do not use variable-length arrays. Use `snprintf`, never `sprintf`, and NULL-check before
  dereferencing.
- Log actionable runtime failures with `log("SYSERR: ...")`; fix all `-Wall -Wextra` warnings.
- Use repository macros and typed VNUM definitions after tracing them; never hard-code virtual
  numbers.

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
- When adding or removing any source or CuTest file, update both `Makefile.am` and `CMakeLists.txt`.

## Special Procedure Refactor

- Preserve the legacy `SPECIAL` ABI, canonical persisted names, world formats, dispatch order,
  scheduling, return behavior, activation flags, and boot precedence until tests authorize change.
- Keep procedure definitions, binding provenance, and invocation gateways as separate
  responsibilities.
- Construct typed event context at call sites while complete data is available; translate legacy
  arguments only at the compatibility seam.
- Treat callback pointers as borrowed for one synchronous invocation. Cache iteration successors
  before handlers that may extract owners or targets.
- World data is the preferred authoring source for new reusable bindings, but compatibility
  precedence remains authoritative until deliberately migrated.
- Preserve unresolved authored names or block their implicit OLC overwrite; a diagnostic alone does
  not prevent data loss.
- Add shared helpers only for a named game rule with at least two audited consumers and focused
  tests.
- Keep artifact ownership, custody, progression, and persistence out of the general spec subsystem.
- Prefer DG Scripts for localized narrative, dialogue, puzzles, and sequencing without engine-level
  lifecycle requirements.

## Local Configuration and Credentials

- Never modify `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`; edit the matching example
  header only when a template change is required.
- Never modify credential-bearing `lib/.env` or `lib/mysql_config` without explicit permission; edit
  `lib/.env.example` or `lib/mysql_config_example` instead.
- Never expose credentials in logs, test output, documentation, commits, or diagnostics.

## Database Layer

- MariaDB/MySQL is required; access it through the established `src/mysql.c` integration and C
  client dependency.
- Keep connection details in the existing credential files and environment, never in source.
- Escape or parameterize untrusted data according to existing database helpers; never concatenate
  raw player input into SQL.
- Keep schema and setup changes under the existing `sql/` and deployment conventions, with tests and
  documentation appropriate to the affected subsystem.

## Testing

- Use root production-linked CuTest coverage for vessel, vehicle, autopilot, special-procedure, and
  other behavior that touches real game sources and structures.
- Run `make test`, then always run `make install`; do not leave a root-level `circle` artifact.
- There is no `test_runner` binary. The focused protocol harness is
  `unittests/CuTest/protocol_parser_tests` and is not a substitute for production-linked coverage.
- Add `unittests/CuTest/test_*.c` files to `cutest_SOURCES` and `cutest_test_files` in `Makefile.am`
  and `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.
- Characterize observable legacy behavior before refactoring it; test behavior and safety contracts,
  not private implementation layout.
- Cover normal and `-s` modes, OLC round trips, boot precedence, aliases, invalid metadata,
  extraction, iteration safety, and exact event translation where applicable.

## Build and Tooling

- Prefer incremental Autotools builds: `make clean`, `make -j$(nproc)`, then `make install`.
- Use CMake as a supported secondary build and keep its manifests behaviorally synchronized.
- Use `.clang-format` for formatting and `.clang-tidy` for configured static analysis.
- Respect the pre-commit hooks, including include-comment alignment changes; rebuild and retest
  after formatting modifies source.

## Documentation

- Update relevant builder guides, helpfiles, architecture, developer, system, and master-index
  documentation when behavior changes.
- Document implemented behavior as current and leave later phases clearly labeled as proposals.
- Keep all documentation ASCII-only UTF-8 with Unix LF line endings.

## Git and Review

- Never attribute AI or assistants in commits or project artifacts.
- Use concise imperative commit messages and keep each commit to one logical, independently testable
  change.
- Review changes against the session base commit and preserve existing authored content and history.

## Local Dev Tools

| Category | Tool | Config |
|----------|------|--------|
| Compiler | GNU-compatible C23 compiler | `configure.ac`, `CMakeLists.txt` |
| Formatter | clang-format | `.clang-format` |
| Static analysis | clang-tidy | `.clang-tidy` |
| Testing | CuTest and protocol parser harness | `Makefile.am`, `unittests/CuTest/Makefile` |
| Build | Autotools/Automake and CMake | `Makefile.am`, `CMakeLists.txt` |
| Git hooks | pre-commit | `.pre-commit-config.yaml` |
| Database | MariaDB/MySQL C client | `src/mysql.c`, `sql/`, `lib/mysql_config` |

## When In Doubt

- Decide from traced repository evidence and document material assumptions.
- Prefer the smallest behavior-preserving change that satisfies the current session objective.
- Rebuild, test, install, and update documentation before declaring implementation complete.
