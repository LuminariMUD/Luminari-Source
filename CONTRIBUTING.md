# Contributing to LuminariMUD

LuminariMUD accepts focused code, content, test, and documentation changes.
Start with the current repository behavior: trace the relevant source and
preserve established compatibility unless the change explicitly migrates it.

## Before You Start

- Set up the repository with the [onboarding checklist](docs/onboarding.md).
- Read the [development guide](docs/development.md) and the detailed
  [developer reference](docs/guides/DEVELOPER_GUIDE_AND_API.md).
- Search existing issues and discuss changes that alter persisted data,
  player-visible mechanics, or deployment behavior.
- Branch from current `master` with a short descriptive name such as
  `fix/health-timeout` or `feature/new-command`.

## Repository Rules

- The supported codebase is GNU C23 with 2-space indentation, Allman braces,
  declarations at the top of blocks, and `/* */` comments.
- Do not mechanically restyle legacy code. Keep lines within 100 columns where
  practical and fix new `-Wall -Wextra` warnings.
- Use bounded string operations such as `snprintf`; check pointers before
  dereference and log internal failures with `log("SYSERR: ...")`.
- Never edit local `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`.
  Change the matching `.example.h` template only when the template contract
  changes.
- Never commit or overwrite `lib/mysql_config` or `lib/.env`. The tracked
  examples are `lib/mysql_config_example` and `lib/.env_example`.
- Add or remove every production or CuTest source in both `Makefile.am` and
  `CMakeLists.txt`.
- Use symbolic VNUM definitions; do not add numeric virtual numbers directly to
  application code.
- Update maintained documentation and database-first help content when user,
  builder, developer, or operator behavior changes.

## Development Workflow

1. Reproduce or trace the current behavior.
2. Make the smallest cohesive change and add production-linked coverage when
   real game structures or behavior are involved.
3. Format only the files you changed with the repository `.clang-format`.
4. Run the appropriate focused checks, then the root gate.
5. Review the diff for credentials, protected paths, generated artifacts, and
   documentation drift.
6. Commit with a concise imperative subject that describes the outcome. The
   history does not require Conventional Commit prefixes.
7. Open a focused pull request describing behavior, compatibility impact,
   tests, documentation, and any operational follow-up.

## Build and Test

For the configured Autotools checkout:

```bash
make clean
make -j"$(nproc)"
make test
make install
```

There is no `test_runner` binary. `make test` builds the root `cutest`
executable against all production sources and also runs the registered shell
checks. Always follow it with `make install`; do not leave a root-level
`luminari` binary.

The focused protocol parser harness is separate:

```bash
cd unittests/CuTest
make protocol-parser
make test-all
```

See the [testing guide](docs/guides/TESTING_GUIDE.md) for CMake, Valgrind,
world-tool, schema, and subsystem-specific gates.

## Documentation and Content

- Documentation and helpfiles must be valid ASCII, UTF-8, and LF text.
- Link to an authoritative document instead of copying the same procedure into
  several files.
- Historical paths in `docs/CHANGELOG.md` and `docs/previous_changelogs/`
  intentionally record the tree as it existed and should not be rewritten.
- World data under `lib/world/` is production content. Validate format and
  references with the maintained world tools before submitting changes.
- Prefer DG Scripts for localized narrative, dialogue, puzzles, and sequencing;
  use C when behavior needs engine state, broad reuse, performance, combat,
  persistence, or lifecycle guarantees.

## Licensing and Provenance

Third-party code, content, and assets must identify their exact source revision,
author or rightsholder, license or permission, modifications, and required
notices. Do not submit material whose source or permission is unclear, and do
not remove inherited copyright, authorship, credit, or license notices.

Changes that alter licensing or provenance must follow the
[inherited code policy](docs/legal/INHERITED_CODE_POLICY.md), including its
whole-tree relicensing gate. An upstream license change does not by itself
authorize a uniform license claim for this repository.

## Pull Request Checklist

- [ ] The change is scoped and its current behavior was traced.
- [ ] New or changed behavior has appropriate tests.
- [ ] `make test` and `make install` pass, or the pull request explains the
      exact reproducible blocker.
- [ ] Both build manifests agree when source membership changed.
- [ ] Relevant docs, help SQL, and changelog entries are current.
- [ ] New third-party material has complete provenance and preserves all
      required notices.
- [ ] No credential, protected local configuration, root `luminari`, or temporary
      validation artifact is included.
- [ ] The pull request explains any migration, rollout, or rollback step.

Project conduct is governed by [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md), and
licensing terms are in [LICENSE](LICENSE).
