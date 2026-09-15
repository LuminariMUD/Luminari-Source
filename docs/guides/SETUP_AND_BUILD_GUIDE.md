# LuminariMUD Setup and Build Guide

## Supported Setup

LuminariMUD builds on Linux and Linux-compatible environments such as Ubuntu
under WSL2. The server requires MariaDB/MySQL and a compiler with the
repository's GNU C23 feature support.

The preferred fresh-install path is:

```bash
./scripts/deployment/deploy.sh
```

That command installs supported dependencies, creates missing local
configuration from examples, provisions MariaDB, initializes minimal world
data, configures Autotools, builds, and installs `bin/luminari`. Inspect exact
flags with `./scripts/deployment/deploy.sh --help`.

## Ubuntu, Debian, and WSL2 Dependencies

```bash
sudo apt-get update
sudo apt-get install -y build-essential git make autoconf automake libtool \
  cmake pkg-config mariadb-server libmariadb-dev libcrypt-dev libgd-dev \
  libevent-dev libcurl4-openssl-dev libssl-dev libjson-c-dev zlib1g-dev \
  mariadb-client curl pandoc gdb valgrind
```

Minimum supported dependency versions are listed in the
[CMake build guide](../development/CMAKE_BUILD_GUIDE.md); both build systems
require the same libraries.

## Existing Configured Checkout

Autotools is preferred for incremental development:

```bash
make -j"$(nproc)"
make -j"$(nproc)" test
make install
```

Incremental builds retain dependency files and rebuild affected source/header users.
Use `make clean` after changing compiler flags or build configuration, or when dependency
files are stale; it is not part of the normal edit/test loop.

Optional compiler caching and faster unoptimized development builds:

```bash
sudo apt install ccache
export PATH=/usr/lib/ccache:$PATH
./configure
# Optional: use this instead when optimizing edit/compile latency.
./configure CFLAGS="-g -O0"
```

Put the `PATH` export in your shell profile so every build and test shell sees it. Do not
configure with `CC="ccache gcc"`: the test gates run `$CC` as a single program name, so
`make test` fails with `ccache gcc: command not found`. The ccache masquerade directory
keeps `CC=gcc` and is what CI uses. Clean once when switching an existing build's compiler
or flags. CI retains its optimized
and instrumented profiles. For CMake, add `-DCMAKE_C_COMPILER_LAUNCHER=ccache` to configure;
`cmake --preset dev` already selects a debug build.

If the generated build files are absent:

```bash
autoreconf -fvi
./configure
make -j"$(nproc)"
make -j"$(nproc)" test
make install
```

`make test` may leave a root-level test build of `luminari`; the required
`make install` step activates the versioned binary at `bin/luminari` and removes
that root artifact.

## Fresh Manual Configuration

Only when the real local files do not exist, copy the tracked examples:

```bash
test -e src/config/campaign.h || cp src/config/campaign.example.h src/config/campaign.h
test -e src/config/mud_options.h || cp src/config/mud_options.example.h src/config/mud_options.h
test -e src/config/vnums.h || cp src/config/vnums.example.h src/config/vnums.h
test -e lib/mysql_config || install -m 600 lib/mysql_config_example lib/mysql_config
test -e lib/.env || install -m 600 lib/.env_example lib/.env
```

Edit local configuration without committing it. Never overwrite an existing
`src/config/campaign.h`, `src/config/mud_options.h`, `src/config/vnums.h`, `lib/mysql_config`, or
`lib/.env`. Database initialization details are in the
[deployment guide](../deployment/DEPLOYMENT_GUIDE.md) and
[database initialization guide](DATABASE_INITIALIZATION_GUIDE.md).

A checkout created before the local headers moved to `src/config/` moves them
once. Until it does, configure, CMake, `deploy.sh`, and `setup.sh` stop and
print this command:

```bash
mkdir -p src/config && mv -n src/{campaign,mud_options,vnums}.h src/config/
```

World and text data must exist under `lib/`. Use the deployment script for a
fresh minimal world rather than assembling the required indexes manually.

## Production Profile

The default configuration is the development build. Production deployments
use the optimized and hardened profile, which both build systems derive from
`scripts/deployment/production_profile.sh`:

```bash
./configure --enable-production
make -j"$(nproc)"
./scripts/deployment/verify_hardened_binary.sh ./luminari
make -j"$(nproc)" test
make install
```

Unknown configure options are fatal, so a misspelled profile cannot fall back
to the default flags. `--enable-lto`, `--with-pgo-generate=DIR`, and
`--with-pgo-use=PATH` are explicit, measured additions. The
[deployment guide](../deployment/DEPLOYMENT_GUIDE.md#production-build-profile)
records the flag policy, verification, and crash-symbolization workflow.

## Compiler Policy and Warning Tiers

The build is GNU C23 on GCC or Clang. Two compiler generations are supported
and both are exercised by blocking CI jobs on every pull request:

| Role | GCC | Clang | Where CI gets it |
| -- | -- | -- | -- |
| Minimum | 13 | 18 | the `ubuntu-latest` runner image |
| Current | 16.2 | 22.1.8 | the `gcc:16.2` container image; apt.llvm.org |

GCC 13 only knows the pre-publication `-std=gnu2x` spelling; configure and
CMake accept it after the C23 keyword probe passes. Update cadence: the
current versions in `.github/workflows/test.yml` move to each new GCC and
LLVM point release within a month of it shipping, and the minimum moves when
the runner image's distribution drops a compiler. Every job that compiles
runs `scripts/ci/check_compiler.sh`, which reads the preprocessor's
predefined macros to prove the family and version before building, so
installing Clang can never silently produce a GCC build. The configure
summary and the CMake status output print the effective warning flags.

GNU extensions are an explicit choice (`-std=gnu23`, `CMAKE_C_EXTENSIONS ON`).
The scheduled `toolchain-analysis.yml` workflow compiles the tree as ISO C23
with `-Wpedantic` and reports how much of it depends on extensions; the
report is informational.

Warnings come in three cumulative tiers. The flag lists live in
`scripts/deployment/production_profile.sh`, which both build systems call so
the two builds apply the same set; each flag is probed, and a compiler that
lacks one reports it and continues.

| Tier | Contents | Enforcement |
| -- | -- | -- |
| `baseline` | `-Wall -Wextra` plus prototype hygiene, format security, `-Wvla`, and the GCC allocation-size and flexible-array checks | errors on every pull request (`--enable-werror`, `LUMINARI_WERROR=ON`) |
| `migration` | conversions, shadowing, switch coverage, missing prototypes, `-Wformat=2`, allocation, duplicated conditions and branches, logical-operator mistakes, fallthrough, `-Wwrite-strings` | a per-compiler budget that may only shrink |
| `analysis` | GCC `-fanalyzer`; Clang's opinionated extras | scheduled; GCC's `-Wanalyzer-*` classes have a budget that may only shrink, the rest is informational (see [Static Analysis](#static-analysis)) |

Select a tier with `./configure --enable-warning-tier=migration` or
`cmake -DLUMINARI_WARNING_TIER=migration`. `-Werror` is refused with any
tier but `baseline`; the migration tier has thousands of pre-existing
instances and is held by `scripts/ci/check_warning_budget.py` instead. The
CI job builds with the pinned current compilers and compares the count of
distinct warning sites per class with `scripts/ci/warning_budget_gcc-16.txt`
and `scripts/ci/warning_budget_clang-22.txt`. Growth in any class, or a new
class, fails the job. Counting is by distinct site, and the build uses
make's per-target output sync so parallel diagnostics cannot interleave and
change the count. After fixing warnings, lower the budget:

```bash
cmake -S . -B build/budget -DCMAKE_C_COMPILER=clang-22 \
  -DLUMINARI_WARNING_TIER=migration -DBUILD_TESTS=ON
cmake --build build/budget -j"$(nproc)" -- --output-sync=target 2>&1 | tee build/budget/build.log
scripts/ci/check_warning_budget.py --compiler clang-22 --log build/budget/build.log --update
```

A class whose budget reaches zero on both compilers is promoted to the
baseline tier. A warning that must be suppressed is suppressed at the site
(`__attribute__` or a pragma) or, for a diagnostic that is wrong for this
code base, in the profile script next to a comment giving the reason; the
repository-wide tier lists are never weakened to accommodate one site.

Feature detection is independent of the warning policy: no configure or
CMake probe uses `-Werror` unless the probe itself requires it, and
`scripts/ci/check_configure_probes.sh` configures both build systems with
strict flags and plainly and fails if the generated `conf.h` differs.

## CMake

CMake is the supported secondary build. Use the checked-in presets, which
enable tests and write to `build/<preset>`:

```bash
cmake --preset dev
cmake --build --preset dev -j"$(nproc)"
ctest -j"$(nproc)" --preset dev
cmake --install build/dev
```

`dev-clang`, `ci-gcc`, `ci-clang`, `analysis`, `sanitizers`, `coverage`,
`release-hardened`, and `cross-aarch64` cover the other supported workflows.
Options such as `LUMINARI_WARNING_TIER`, `LUMINARI_WERROR`, and
`LUMINARI_SANITIZERS` are documented in the [CMake build guide](../development/CMAKE_BUILD_GUIDE.md).

Both build systems must list the same sources. `make check-build-parity` (also
run by `make test`, CTest, and CI) fails when `Makefile.am` and
`CMakeLists.txt` drift, including the production-source membership of both
`cutest` targets. `make distcheck-archive` configures, builds, tests, and
installs an untouched `git archive HEAD` through both systems; CI runs it as
a blocking job, and it is not part of `make test` because it rebuilds the
tree twice.

The production profile replaces `CMAKE_BUILD_TYPE`:

```bash
cmake -S . -B build -DLUMINARI_PRODUCTION=ON
```

`LUMINARI_LTO`, `LUMINARI_PGO_GENERATE`, and `LUMINARI_PGO_USE` mirror the
Autotools options.

## Run and Verify

```bash
./bin/luminari -d lib
```

The checked-in runtime configuration defaults to the reserved local game port
4100\. While the server runs, verify the loopback health listener from another
terminal:

```bash
./scripts/operations/healthcheck.sh
```

See [development.md](../development/README_development.md) for daily commands,
[TESTING_GUIDE.md](TESTING_GUIDE.md) for all test surfaces, and
[incident-response.md](../runbooks/incident-response.md) for operational
diagnosis.

## Source Tree Hygiene

The repository tracks source and data only. `scripts/ci/check_source_hygiene.py`
enforces three rules over every tracked file, reports every violation, and exits
non-zero when any is found:

- **No build products.** ELF, PE, Mach-O, and ar files are rejected by magic
  bytes; object, library, coverage, profiler, and core-dump files by name; and
  configure, automake, and CMake outputs by name. Images, audio, and fonts are
  the only binary files allowed.
- **Well-formed text.** Every non-media file must be valid UTF-8 with no NUL
  bytes and no carriage returns. This check is independent of the ASCII rule so
  a malformed byte or CRLF file cannot hide behind it.
- **ASCII documentation.** Every `*.md` file, plus `*.txt` under `docs/`, must
  be plain ASCII. Use `->`, `-`, straight quotes, `[OK]`/`[X]`, and ASCII box
  drawing (`.-|+'`) instead of typographic punctuation, emoji, or Unicode boxes.
  Documentation that has to describe a Unicode glyph names its code point
  (`U+2588 full block`) rather than embedding it.

Exceptions to the ASCII rule live in `ASCII_EXCEPTIONS` inside the script with
the reason each one exists. The list is empty: legal text under `docs/legal/`
and every current document are already ASCII. HTML under `docs/` declares its
own charset, so it is held to the UTF-8 and LF rules only; of it, only the
pandoc builder guides and the spell pages are generated. Non-ASCII in C sources is limited to deliberate in-game glyphs
(map symbols, box borders) and is outside the documentation rule.

Run the checks locally:

```bash
python3 scripts/ci/check_source_hygiene.py              # tracked files
python3 scripts/ci/check_source_hygiene.py --root DIR   # an unpacked make dist tree
pre-commit run source-hygiene --all-files
```

CI runs the tracked-file check on every push (`.github/workflows/hygiene.yml`).
The behavioral test job additionally proves that configure, build, test,
install, and clean leave `git status` empty and that `make dist` produces a
tarball that passes the same scan. `.editorconfig` and `.gitattributes` carry
the matching editor and Git settings (UTF-8, LF, 2-space C indentation, text
world files, binary media).

## Formatting

Every hand-maintained text file type has one pinned formatter. The pre-commit
hooks in `.pre-commit-config.yaml` run them on staged files, and the Code Quality
workflow (`.github/workflows/quality.yml`) runs every hook over every tracked
file. Install the hooks once per clone with `pre-commit install`.

| Files | Formatter | Hook id | Settings |
| -- | -- | -- | -- |
| C and C headers | clang-format 18.1.8 | `clang-format` | `.clang-format` |
| Python | ruff 0.16.7 | `ruff-format` | `ruff.toml`: 100 columns, 4-space indentation |
| Shell | shfmt 3.14.1 | `shfmt` | `.editorconfig`: 2-space indentation, indented `case` branches |
| SQL | sqlfluff 4.3.0, layout rules only | `sqlfluff-fix` | `.sqlfluff`, `.sqlfluffignore` |
| Markdown | mdformat 1.0.0 with the gfm, frontmatter, and simple-breaks plugins | `mdformat` | `.mdformat.toml`: prose is never re-wrapped |
| YAML, JSON, HTML, CSS, JavaScript | prettier 3.9.6 | `prettier` | `.editorconfig` width, `.prettierignore` |
| CMake | gersemi 0.29.1 | `gersemi` | `.gersemirc` |
| PHP | php-cs-fixer 3.95.25, PER Coding Style 3.0 | `php-cs-fixer` | `.php-cs-fixer.dist.php` |
| PowerShell | PSScriptAnalyzer 1.25.0 `Invoke-Formatter` | `powershell-format` | `PSScriptAnalyzerSettings.psd1` |

Makefiles, Dockerfiles, and TOML have no formatter; `.editorconfig` and the
hygiene hooks cover their whitespace.

Format every file of one type through its hook, never by running the tool
directly: `pre-commit run <hook-id> --all-files`. The hook applies the
repository's exclusions and skips tracked symlinks, which shfmt would otherwise
replace with copies.

### Formatter runtimes

The hooks install their own tool environments on first use, except two that need
a runtime on `PATH`. Only commits that stage those file types, and
`pre-commit run --all-files`, need them:

- `php-cs-fixer` needs PHP 8.3: `sudo apt-get install -y php8.3-cli`.
- `powershell-format` needs PowerShell 7 from Microsoft's package repository:

```bash
wget -q https://packages.microsoft.com/config/ubuntu/24.04/packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb && rm packages-microsoft-prod.deb
sudo apt-get update && sudo apt-get install -y powershell
```

`scripts/development/format_php.sh` downloads the pinned php-cs-fixer phar once
and checks its sha256 before every run, holding a lock on the cache so runs
from checkouts that share it take turns; a phar that does not match is
downloaded again, and a download that does not match fails the hook without
running. `scripts/development/format_powershell.ps1` saves PSScriptAnalyzer
once. Both keep them in
`${LUMINARI_FORMATTER_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/luminari-formatters}`.
GitHub's Ubuntu 24.04 runner has both runtimes, and the local CI image installs
them.

### Files that are never formatted

- `docs/previous_changelogs/`: dated historical records (the mdformat hook
  excludes them).
- `lib/WILD_KB.md`: written by the wilderness knowledge-base command (the
  mdformat hook excludes it).
- Listed in `.prettierignore`: the architecture maps and their sources, sealed
  by the sha256 receipts in
  [`docs/architecture-maps/README.md`](../architecture-maps/README.md); the
  conversion run records under `lib/rol-conversion/runs/`; the pandoc builder
  guides under `docs/web/guides/` and their template,
  `docs/web/assets/pandoc-template.html`, which
  `scripts/development/generate-web-guides.sh --check` compares; the spell pages
  written by `util/generate_spell_html.sh` and
  `util/generate_spell_html_detailed.py`; and
  `scripts/world/wtool_constants.json`, which
  `wtool.py constants sync --check` compares.
- Listed in `.sqlfluffignore`: 18 legacy SQL files that the MariaDB dialect
  cannot parse (see below).
- World files, `lib/text/help/help.hlp`, and the legal archive have no
  formatter: they are written by OLC, hedit, or tools, or kept byte-identical.

### SQL format rules

sqlfluff applies its layout rules only. The capitalisation rules would rename
case-sensitive table identifiers, LT05 (line length) cannot always be fixed
automatically, and `end-of-file-fixer` owns final newlines. `disable_noqa = True`
keeps an inline `-- noqa` from hiding a parse error or a formatting change.

New SQL cannot opt out of the formatter. `.sqlfluffignore` lists the legacy
files that do not parse; the list may shrink but never grow.
`scripts/ci/check_sql_format_policy.py` runs as the always-run
`sql-format-policy` hook and as its own Code Quality step. It fails on a
`.sqlfluffignore` entry outside the frozen list, sqlfluff configuration anywhere
but the repository root, changed `.sqlfluff` settings, an inline `sqlfluff:`
comment in a SQL file, a missing or narrowed sqlfluff hook, and a top-level
`files` or `exclude` pattern in `.pre-commit-config.yaml` that keeps a SQL file
from the hooks. `--self-test` proves each case is rejected.

Write new SQL in forms the MariaDB dialect parses:

| Does not parse | Write instead |
| -- | -- |
| `DELIMITER` blocks for procedures, functions, and compound triggers | create the routine from C in `src/database/db_init.c` (see [DATABASE_INITIALIZATION_SYSTEM.md](../systems/DATABASE_INITIALIZATION_SYSTEM.md)); a single-statement `CREATE TRIGGER ... FOR EACH ROW SET ...;` parses |
| `CREATE VIEW IF NOT EXISTS` | `CREATE OR REPLACE VIEW` |
| `WHERE BINARY tag = 'x'`, `ON BINARY a = b` | `CAST(tag AS BINARY) = 'x'` |
| `SHOW INDEX FROM t` | a query on `information_schema.statistics` |
| `SOURCE other.sql` | apply each file separately |
| `DEFAULT (expression)` | a literal default, or set the value where rows are written |

`ADD COLUMN IF NOT EXISTS`, `CREATE INDEX IF NOT EXISTS`,
`ON DUPLICATE KEY UPDATE`, `PREPARE` and `EXECUTE`, `CREATE EVENT IF NOT EXISTS`,
`DROP PROCEDURE IF EXISTS`, `COLLATE utf8mb4_bin`, and table options such as
`ENGINE=InnoDB DEFAULT CHARSET=utf8mb4` parse as written.

sqlfluff prints `FAIL` with an LT02 note for the multi-table `UPDATE ... JOIN`
in `sql/components/vessels_harbor_sandbox.sql`. The hook still exits 0: the note
is lint output, not a formatting failure.

### Builder guide pages

pandoc renders `docs/web/guides/*.html` from
`docs/world_game-data/OEDIT_GUIDE.md`, `MOB_FLAGS.md`, and `ROOM_FLAGS.md`. After
editing one of those sources, let the mdformat hook format it first, then run
`scripts/development/generate-web-guides.sh`: pandoc reads the formatted
Markdown, and `wtool.py docs --check` fails until the pages are regenerated.

### Upgrading a formatter

`pre-commit autoupdate` bumps the `rev` pins. Bump the mdformat plugin versions
in `additional_dependencies` and the PyYAML pin by hand, php-cs-fixer by changing
the version and sha256 in `scripts/development/format_php.sh`, and
PSScriptAnalyzer by changing the version in
`scripts/development/format_powershell.ps1`. Then run
`pre-commit run --all-files`, commit the result as a formatting-only commit, and
list that commit in `.git-blame-ignore-revs`. After a sqlfluff upgrade, also run
sqlfluff on the `.sqlfluffignore` entries and remove any that now parse from both
`.sqlfluffignore` and the frozen list in `check_sql_format_policy.py`. Rebuild the
local CI image after any hook change, as [TESTING_GUIDE.md](TESTING_GUIDE.md)
describes.

### Blame

`.git-blame-ignore-revs` lists the commits that only reformatted files. GitHub's
blame view skips them; to make `git blame` skip them too, run this once per
clone:

```bash
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

### Branches that predate the formatters

A branch that forked from `master` before the formatters landed can merge
`master` as it is, but each conflict then also shows the reformatted code around
the branch's edit. To leave only the edits in conflict, format the branch's own
files with `master`'s settings first, then merge:

```bash
git fetch origin
git checkout origin/master -- .pre-commit-config.yaml .editorconfig ruff.toml \
  .sqlfluff .sqlfluffignore .mdformat.toml .prettierignore .gersemirc \
  .php-cs-fixer.dist.php PSScriptAnalyzerSettings.psd1 \
  scripts/development/format_php.sh scripts/development/format_powershell.ps1 \
  scripts/ci/check_sql_format_policy.py
pre-commit run --files $(git diff --name-only --diff-filter=d origin/master...HEAD)
git commit -am "Format with the repository formatters"
git merge origin/master
```

The commit stages the PHP and PowerShell settings files, so it needs both
runtimes. Rebasing instead of merging replays the branch's earlier, unformatted
commits, so their conflicts include the reformatted code again.

## Static Analysis

Four gates hold static analysis at its current findings. Each keeps a baseline
of the findings that predate it: a new finding fails CI, and a baseline may only
shrink.

| Gate | Tool | Runs in | Baseline |
| -- | -- | -- | -- |
| clang-tidy, including the Clang static analyzer | clang-tidy 22.1.8, pinned in `scripts/ci/clang-tidy-requirements.txt` | Code Quality: the translation units each pull request or push changes; the whole tree weekly and on manual runs | `scripts/ci/clang_tidy_baseline.txt`, findings per file and check |
| GCC static analyzer | GCC 16.2 `-fanalyzer` in the `gcc:16.2` image | Toolchain analysis: the server, weekly | `scripts/ci/warning_budget_gcc-16-analyzer.txt`, sites per `-Wanalyzer-*` class |
| Header self-containment | the configured compiler | `make test` and CTest, so every CI job that runs them | `scripts/ci/header_self_containment_baseline.txt`, headers that do not compile alone yet |
| CodeQL source coverage | CodeQL with the `security-extended` queries | Security | none: every production source must be in the database |

Findings depend on the configuration headers. CI and every baseline use the
`src/config/*.example.h` templates, so a checkout with customized local headers
can report different findings. `python3 scripts/ci/local/run.py --job quality-clang-tidy`
reproduces the clang-tidy job exactly, including its changed-unit selection.

### clang-tidy

`.clang-tidy` is the only configuration: the enabled checks, their options, and
each disabled check with its scope, reason, owner, and expiry.
`scripts/ci/check_clang_tidy.py` passes it with `--config-file` and refuses any
clang-tidy but the pinned release.

The compilation database comes from `cmake --preset analysis`, which configures
with Clang and exports `build/analysis/compile_commands.json` without building:
the server, the production-linked test suite, and the utilities. The check
refuses a database written by another compiler, whose flags clang-tidy would
report, and one without a command for every C source in `luminari_SOURCES` and
`cutest_test_files`. A source that also builds into `cutest` is analyzed once,
with the server's command.

Findings are distinct (file, line, column, check) sites, counted per file and
check. A count above the baseline, including a check that appears in a file for
the first time, fails and prints every site of that file and check; a lower
count is reported. `--base REF` analyzes the sources changed since `REF` and
every source that includes a changed header, and compares a header's counts only
when all of its includers were analyzed. A change to `.clang-tidy`, the CMake
files, the pin, the baseline, the check itself, or the production profile
analyzes the whole tree. Pull requests and pushes run with `--base HEAD^1`; the
weekly and manual runs analyze everything.

Each run writes the complete clang-tidy output (`clang-tidy.log`) and a JSON
report (`clang-tidy-report.json`: tool version, mode, analyzed units, every
finding, and the failures) to `--report-dir`, by default
`build/analysis/clang-tidy-report`. CI uploads it as the `clang-tidy-report`
artifact, also when the check fails.

```bash
python3 -m venv .venv
.venv/bin/python -m pip install -r scripts/ci/clang-tidy-requirements.txt
cmake --preset analysis   # add -DCMAKE_C_COMPILER=clang-18 where there is no clang command
# The pull-request check: sources changed since the merge base.
.venv/bin/python scripts/ci/check_clang_tidy.py --build-dir build/analysis \
  --clang-tidy .venv/bin/clang-tidy --base "$(git merge-base origin/master HEAD)"
# The weekly check: the whole tree.
.venv/bin/python scripts/ci/check_clang_tidy.py --build-dir build/analysis \
  --clang-tidy .venv/bin/clang-tidy
# After fixing findings, record the lower counts from a whole-tree run.
.venv/bin/python scripts/ci/check_clang_tidy.py --build-dir build/analysis \
  --clang-tidy .venv/bin/clang-tidy --update
```

Fix a new finding. For a false positive, put
`/* NOLINTNEXTLINE(check-name) -- reason */` on the line above it. The check
fails a suppression that does not name its checks, uses a wildcard, names a
check the pinned release does not have, or gives no reason after `--`. A check
that is wrong for the whole code base is disabled in `.clang-tidy` instead, with
its scope, reason, owner, and expiry. `--update` refuses to raise a count; when
a file moves, rename its baseline entries in the same change. To adopt a new
clang-tidy release, change the pin, delete the baseline, run `--update` to record
the release's first baseline, and review that diff like any other change in
findings.

### GCC static analyzer

The weekly `analysis` job in `toolchain-analysis.yml` builds the server with the
analysis warning tier. With GCC 16.2 it compares the `-Wanalyzer-*` classes with
their budget, which also refuses a build log that contains a compiler error; the
other analysis-tier classes, the Clang 22 build, and the ISO C23 extension report
are informational. `src/character/class.c` is compiled without the analyzer,
which needs more than 30 GiB for it; the exclusion in `Makefile.am` and
`CMakeLists.txt` records its owner and expiry.

```bash
rm -rf /tmp/luminari-gcc-analysis && mkdir /tmp/luminari-gcc-analysis
git archive HEAD | tar -x -C /tmp/luminari-gcc-analysis
for header in campaign mud_options vnums; do
  cp /tmp/luminari-gcc-analysis/src/config/$header.example.h \
    /tmp/luminari-gcc-analysis/src/config/$header.h
done
docker build -t luminari-ci:local-gcc-16.2 -f scripts/ci/local/Dockerfile.gcc-16.2 .
docker run --rm --user "$(id -u):$(id -g)" -v /tmp/luminari-gcc-analysis:/src -w /src \
  luminari-ci:local-gcc-16.2 bash -c '
    cmake -S . -B build/analysis -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DLUMINARI_WARNING_TIER=analysis -DBUILD_TESTS=OFF &&
    cmake --build build/analysis -j8 --target luminari -- -k --output-sync=target \
      >build/analysis/build.log 2>&1'
scripts/ci/check_warning_budget.py --compiler gcc-16-analyzer --classes '^analyzer-' \
  --log /tmp/luminari-gcc-analysis/build/analysis/build.log
```

After fixing analyzer findings, add `--update` to the last command.

### Header self-containment

Every header under `src/` must compile on its own: a translation unit holding
only `#include "dir/header.h"` must pass `-fsyntax-only -Werror` with nothing but
the build root (for `conf.h`) and `src/` on the include path.
`scripts/ci/header_self_containment_baseline.txt` lists the headers that do not
yet. Any other header that fails fails `make test` and CTest, which run the check
with the configured compiler; a listed header that now compiles alone is
reported. Compilers disagree about a few default diagnostics (GCC 14 accepts 24
listed headers that GCC 13 and 16 and Clang 18 and 22 reject), so the list is the
union over the supported compilers, and `--update` drops only the headers that
pass with every compiler it is given.

```bash
make test-header-self-containment
ctest --test-dir build/dev -R header-self-containment
# After fixing headers:
scripts/ci/check_header_self_containment.py --build-root . \
  --cc gcc-13 --cc gcc-16 --cc clang-18 --cc clang-22 --update
```

### CodeQL source coverage

After CodeQL analyzes the traced build, `scripts/ci/check_codeql_coverage.py`
reads the database's source archive and fails when a C source in
`luminari_SOURCES` is missing, so a source the traced build skipped cannot drop
out of the results unnoticed. With the CodeQL CLI, from a configured checkout:

```bash
make clean
codeql database create /tmp/luminari-codeql --language=cpp --command='make -j8'
scripts/ci/check_codeql_coverage.py --database /tmp/luminari-codeql
```

### Suppressions

A suppression names the diagnostic and records its scope, reason, owner, and
expiry beside it:

| Suppression | Where it is recorded |
| -- | -- |
| A clang-tidy check, everywhere | `.clang-tidy` |
| One clang-tidy finding | `/* NOLINTNEXTLINE(check) -- reason */` on the line above, enforced by the clang-tidy check; it expires when the check stops reporting that line |
| A compiler warning in one block | a comment above the `#pragma GCC diagnostic push` |
| A warning flag in a build profile | a comment beside it in `scripts/deployment/production_profile.sh` |
| The GCC analyzer for one file | the comment beside the exclusion in `Makefile.am` and `CMakeLists.txt` |
| A block clang-format must not reflow | a comment above `/* clang-format off */` |
| Findings that predate a gate | that gate's baseline, which may only shrink |
