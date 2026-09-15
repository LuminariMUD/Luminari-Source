# Pre-commit formatters for every maintained file type

Status: Steps 1 to 9 are done and verified on `chore/pre-commit-clang-tidy`;
Step 0's host runtimes and Step 10 remain for the owner. Progress below is the
resume point. The plan was written 2026-09-15 on the development host
(`APP_ENV=development`) against `570193508`, the tip of `master`, and revised
the same day with the owner's decisions: Python at 4-space indentation,
mdformat tuned for this repository, the 18 unparseable legacy SQL files frozen
while every other SQL file is enforced, and PHP and PowerShell formatted with
their own runtimes. Every number and result in the plan was measured on
scratch copies and a scratch clone of that revision with the exact versions
pinned here; planning modified no checkout and installed nothing on the host.

Goal: every hand-maintained text file type is formatted by one pinned tool that
runs in the pre-commit hook and in CI, with its settings in one place, no
behavior change, and no rewriting of generated, sealed, or archival files. New
SQL cannot opt out.

## Progress

One commit per step on `chore/pre-commit-clang-tidy`, following Implementation
sequence; `git log --oneline 570193508..` lists them. Each row records what
was verified before its commit.

| Step | State | Commit | Verified |
| -- | -- | -- | -- |
| 1 Python | done | Format Python with ruff | 121 files, +39,840/-38,472; AST identical 121/121; all 542 world-tool tests pass before and after; `wtool.py constants sync --check` passes |
| 2 Shell | done | Format shell scripts with shfmt | 64 files, +4,209/-4,461; AST identical 63/64, the other being the planned glob rewrite; `bash -n` passes for all 73 regular scripts; the 8 symlinks and the scripts' exec bits intact; pubsub retirement, rename static, and background help checks pass |
| 3 SQL | done | Format SQL with sqlfluff and keep new SQL under it | 106 files, +4,038/-2,755; token streams identical 106/106; no frozen file changed; the policy self-test rejects 10 bypasses and each of the six bypass trials fails the check; the hook trials behave as planned; the master schema and all 61 applied components load into MariaDB 10.11 as in `integration.yml`; rename static (through `make`), background help, and pubsub retirement checks pass |
| Markdown prep | done | Prepare Markdown for mdformat | 33 documents and the 2 regenerated guides, +136/-129; mdformat on the result adds 69 escapes, all in prose: the 28 bracket pairs, 7 footnote asterisks in `gear_guide.md`, and 6 in `phase01_test_results.md` (footnote marks and the `A*` name); `wtool.py docs --check`, `generate-web-guides.sh --check`, `check-dg-docs.py`, and source hygiene pass |
| 4 Markdown | done | Format Markdown with mdformat | 192 documents besides this plan, +6,172/-1,784, each byte-identical to a verification clone; guides regenerated, +1,517/-846; the only escapes added are the 69 accepted prose ones; `wtool.py docs --check`, `generate-web-guides.sh --check`, `check-dg-docs.py`, and source hygiene (1,693 files) pass; all 542 world-tool tests pass; the `CLAUDE.md` and `GEMINI.md` symlinks, the changelogs, and `lib/WILD_KB.md` untouched |
| 5 prettier | done | Format YAML, JSON, HTML, CSS, and JavaScript with prettier | 32 of the 44 files in scope, +8,420/-4,024; `prettier --debug-check` passes on all 44 as they were; the parsed YAML or JSON of all 14 reformatted data files is unchanged; `clang-format --dump-config` and `clang-tidy --dump-config` print the same output (only quote style changed); `check-dg-docs.py`, `wtool.py constants sync --check`, `wtool.py docs --check`, the SQL format policy, and all 542 world-tool tests pass |
| 6 CMake | done | Format CMake with gersemi | 2 files, +487/-301; `gersemi --safe` passes and matches the hook's output; `check_build_parity.py` passes, its parser returns identical sources for the old and new `CMakeLists.txt`, and the old pattern no longer finds `cutest` in the new file; a plain configure and the `ci-gcc` preset each generate identical compile commands (343 and 747), targets (1,054 and 1,273), and test commands (29) from both files; the `ci-gcc` preset builds `luminari` and `cutest` |
| 7 PHP | done | Format PHP with php-cs-fixer | 7 files, +278/-250; `php -l` clean and compiled opcodes identical in 7 of 7 (PHP 8.3 image); `.php-cs-fixer.dist.php` lints clean; a second run finds nothing; with an empty cache the wrapper downloads the phar and verifies its sha256, and a download that does not match fails with exit 1 without replacing the cached phar; all 9 files of the step byte-identical to the dry run |
| 8 PowerShell | done | Format PowerShell with PSScriptAnalyzer | 5 scripts, +18/-18, and the settings file, formatted by its own hook; parser tokens identical in 5 of 5 (case-insensitive outside strings and comments); a second run changes nothing; LF endings, final newlines, no byte-order marks; with an empty cache the wrapper saves PSScriptAnalyzer 1.25.0 and gives the same output; all 7 files of the step byte-identical to the dry run |
| 9 CI, image, docs | done | Run every format hook in CI and document the formatters | `quality.yml` runs on every push and pull request to `master`, prints `php --version` and `pwsh --version`, runs `pre-commit run --all-files --show-diff-on-failure`, then the policy check with and without `--self-test`; on the host those commands pass twice with no changes; the rebuilt local CI image has PHP 8.3.6, PowerShell 7.6.6, and `LUMINARI_FORMATTER_CACHE` for uid 1000; `AGENTS.md`, `CONTRIBUTING.md`, `SETUP_AND_BUILD_GUIDE.md` (new Formatting section), and `TESTING_GUIDE.md` updated; hygiene, `wtool.py docs --check`, and all 542 world-tool tests pass; container matrix: see the note below |
| 10 after merge | pending |  | rebase-merge, then one commit adds `.git-blame-ignore-revs` and deletes this plan |

Notes for whoever resumes:

- Change counts match the dry run exactly, except that a supporting change
  (+1/-1) is counted in the step that makes it, not in the formatting step.
- Exec-bit checks must look only at files a step touched: `core.fileMode` is
  false here, and 62 unrelated tracked files (C sources, docs, `.gitkeep`) are
  already 100755 in the index but not executable on disk.
- Step 0 is not done: this host has no passwordless sudo, so neither runtime
  is installed system-wide. Until the owner runs the Step 0 commands, PHP runs
  through a `php` shim first on `PATH`:
  `exec docker run --rm -i -u "$(id -u):$(id -g)" -v "$PWD:$PWD" -v "$LUMINARI_FORMATTER_CACHE:$LUMINARI_FORMATTER_CACHE" -e LUMINARI_FORMATTER_CACHE -w "$PWD" php:8.3-cli php "$@"`
  (its output is byte-identical to Ubuntu's `php8.3-cli`, see PHP), and
  PowerShell 7.6.6 runs from the extracted release archive.
- Found during implementation, a sixth SQL bypass: a top-level `exclude` or
  `files` pattern in `.pre-commit-config.yaml` applies to every hook. With
  `exclude: ^sql/new\.sql$` the sqlfluff hook reports "(no files to check)" and
  exits 0. Step 3 closed it in the policy check (see SQL enforcement).
- `lib/WILD_KB.md` is generated: `generate_wilderness_knowledge_base()` in
  `src/wilderness/wilderness_kb.c` writes it for a wizard command. The Goal
  rules out rewriting generated files, so the mdformat hook excludes it next
  to the changelogs, and the content prep left it alone. The plan had
  counted it among the formatted files.
- The content prep went slightly past the plan's list, each for a reason
  found while tracing the escapes: every `**%cmd% <args>**` item and both
  operator lists in `SCRIPTING_SYSTEM_DG.md` now use code spans, since GitHub
  hid `<vnum>`-style arguments as unknown HTML tags and half-converted lists
  would be inconsistent; the CuTest prototypes got a `c` fence; `*.h` in
  bold text in `CLAUDE.example.md` had broken the emphasis; and two typos
  surfaced, a stray `]` in `LUMINARI_OVERVIEW.md` and an unclosed `*` in
  `INQUISITOR_PERKS.md`.
- `docs/web/data/objects.json` stays formatted: `util/export_objectdb_to_json.py`
  writes real data there, but the tracked file is hand-written demo data.
  The ROL converter hashes its JSON inputs (`_CODE_EVIDENCE_PATHS` in
  `rol_phase8.py`) only to compare them within one run, so reformatting them
  breaks nothing.
- The local CI Dockerfile installs PHP and PowerShell in one layer after the
  LLVM layer, instead of adding `php8.3-cli` to the first apt list, so both
  formatter runtimes sit under one comment and the base layers stay cached.
- This plan is deleted in Step 10, not Step 9: rebase-merging gives the
  formatter commits new SHAs, so `.git-blame-ignore-revs` can only list them
  after the merge, and this document is the reference until then. Its lasting
  content is already in the setup guide's Formatting section.
- Final verification: `pre-commit run --all-files` passed twice on the final
  tree on the host with no changes, and the local CI matrix
  (`scripts/ci/local/run.py --jobs 4 --cpus 4` on a rebuilt
  `luminari-ci:local-fast`) passed all 28 jobs on `b68b8c382` in 399 s: the 23
  `test.yml` jobs (world tools, build parity, the eight CMake builds, warning
  budgets, clean archives, unit tests, production profiles, sanitizers,
  memory check, coverage), both `integration.yml` jobs (the master schema and
  every applied component load into MariaDB), the `quality.yml` format check
  with all 17 hooks, hygiene, and gitleaks.
- The first container run of the quality job failed: `format_php.sh` had been
  committed as 100644. `core.fileMode` is false here, so `git add` ignores
  the exec bit of a new file, and the hook passed on this host only because
  the file on disk was executable. A follow-up commit sets 100755 with
  `git update-index --chmod=+x`; check new scripts with `git ls-files -s`.
- GitHub's `test.yml` fails the `CMake gcc-16` Test step on this branch with
  `autorun-supervision` and `vessel-memory-analyzer`. `master` fails the same
  two tests at `570193508`, and both pass in the local `gcc:16.2` image, so the
  failure predates this work.
- PR #190 adds `sql/components/help_crafting_entries.sql` and
  `verify_help_crafting_entries.sql`. Both parse under sqlfluff 4.3.0 with this
  `.sqlfluff`, so that branch can take the hooks through the rebase recipe in
  Step 0 without an exemption.

## Verdict

Worth doing. Only C and C headers are formatted today (clang-format v18.1.8 in
the hook and in the `quality.yml` Format Check job). Nothing enforces layout
for the rest: a formatter would change 121 of 126 Python files, 64 of 73 shell
scripts, 106 of 139 SQL files, all 7 PHP files, and all 5 PowerShell scripts.
Every formatter chosen below was proven behavior-preserving on this tree, and a
dry run of the complete end state passed every hook on a second run and every
affected consumer check.

Costs:

- A one-time reformat of 534 files, about 118,000 changed lines (insertions
  plus deletions), two thirds of it Python. It lands as one commit per
  language.
- Conflicts for branches that are open when it lands (recipe in Step 0).
- Hosts that commit PHP or PowerShell need `php8.3-cli` and PowerShell 7
  (Step 0 installs both here; GitHub's Ubuntu 24.04 runner already has them).
- Two formatter wrapper scripts, one SQL policy check, and six supporting
  changes that the dry runs proved necessary.
- The first commit after installing the hooks downloads the tool environments
  (under a minute here).

## Coverage decisions

| Type | Files | Tool and pin | Decision | One-time change | Proof on this tree |
| -- | -- | -- | -- | -- | -- |
| C, C headers | 697 | clang-format v18.1.8 (existing) | Unchanged | none | `pre-commit run --all-files` already passes |
| Python | 126 | ruff v0.16.7 `ruff-format`, 4-space | Format | 121 files, +39,841/-38,473 | AST identical in 121 of 121 |
| Shell | 73, plus 5 symlinks | shfmt v3.14.1 | Format | 64 files, +4,210/-4,462 | AST identical in 62 of 64; the other 2 are planned edits |
| SQL | 139 | sqlfluff 4.3.0, layout rules only | Format 121; freeze 18; enforce all new SQL | 106 files, +4,038/-2,755 | token streams identical in 106 of 106 |
| Markdown | 240 | mdformat 1.0.0 with mdformat-gfm 1.0.0, mdformat-frontmatter 2.1.2, mdformat-simple-breaks 0.1.0 | Format, except dated changelogs and the generated `lib/WILD_KB.md` | 194 files, +6,376/-1,954 (before excluding `lib/WILD_KB.md`) | mdformat's render check; idempotent |
| YAML, JSON, HTML, CSS, JavaScript | 44 in scope | prettier 3.9.6 | Format; generated files ignored | 32 files, +8,420/-4,024 | `prettier --debug-check` passes on all 44 |
| CMake | 2 | gersemi 0.29.1 | Format | 2 files, +487/-301 | parity parser output identical after one regex fix |
| PHP | 7 | php-cs-fixer 3.95.25, `@PER-CS3x0` | Format | 7 files, +278/-250 | `php -l` clean and compiled opcodes identical in 7 of 7 |
| PowerShell | 5 | PSScriptAnalyzer 1.25.0 `Invoke-Formatter` | Format | 5 files, +18/-18 | parser tokens identical in 5 of 5 |
| Pandoc builder guides | 3 | existing generator, pandoc 3.1.3 | Regenerate, never format | 3 files, +1,517/-846 | `wtool.py docs --check` passes |
| World files, help, run records, receipts | - | none | Never formatted | none | written by OLC, hedit, or tools, or sealed by hashes |
| Makefiles, Dockerfiles, TOML | - | none | Not formatted | none | no mature formatter; `.editorconfig` and hygiene hooks apply |

## Evidence and chosen settings

### Python: ruff formatter, 4-space indentation

- `ruff.toml` sets only `line-length = 100`, the C column limit. Indentation
  stays at ruff's default of 4, which `.editorconfig` already declares for
  Python. 93 of 126 files use 2-space indentation today, so most of the Python
  churn is re-indentation.
- The upstream `ruff-format` hook also accepts Markdown and would format Python
  code blocks inside 4 documents. The hook is limited to `types_or: [python]`
  so each file type has exactly one formatter.
- Proof: for all 121 changed files the old and new Python ASTs are identical
  once docstring indentation is normalized, and a second pass changes nothing.
  In the full dry run the only AST difference is the planned
  `check_build_parity.py` pattern change.

### Shell: shfmt configured by .editorconfig

- shfmt reads `.editorconfig` when it gets no style flags, and the upstream
  hook passes only `--write`. `[*]` already sets 2-space indentation; a new
  `[*.sh]` section adds `switch_case_indent = true`. Of the five layouts
  measured, that one changes the fewest lines.
- `scripts/events/test_pubsub_retirement.sh` line 23,
  `retired_sources=("$project_root"/src/pubsub/*.[ch])`, is valid bash that
  shfmt cannot parse (it reads `[ch]` as an array index). Rewrite it as
  `retired_sources=("$project_root"/src/pubsub/*.c "$project_root"/src/pubsub/*.h)`.
  The script only counts the matches, and it passes after the change.
- Bulk-format through pre-commit, never `xargs shfmt -w`. shfmt replaces a
  symlink with a regular file when it writes (it turned `autorun.sh` into a
  copy in an early dry run), while pre-commit skips the eight tracked symlinks.
  The pinned shfmt 3.14.1 also places `then` after a heredoc differently from
  the host's 3.8.0, so only the pinned version may produce the bulk commit.
- Proof: `shfmt --to-json` ASTs with positions removed are identical for 62 of
  64 changed scripts; the other two are the glob rewrite above and the rename
  test's expected string (see SQL). `bash -n` passes for every script.

### SQL: sqlfluff format, layout rules only

- sqlfluff understands MariaDB (`dialect = mariadb`) and leaves `DELIMITER`
  lines intact. The alternative, sql-formatter 15.8.2, joined `END $$` and
  `DELIMITER ;` onto one line in 5 files, which breaks loading through the
  mysql client, and could not parse 2 files.
- `.sqlfluff` excludes these rules:
  - CP01 to CP05: `sqlfluff format` otherwise changes identifier case
    (`ship_room_templates` to `Ship_room_templates`, `information_schema` to
    `Information_schema`, `TABLE_NAME` to `Table_name`), and table names are
    case-sensitive on Linux MariaDB.
  - LT05 (line length): its violations cannot always be fixed and otherwise
    make the hook fail on files that are already formatted.
  - LT12 (end of file): three SQL files are empty
    (`sql/components/region_resource_effects.sql`,
    `region_resource_effects_simple.sql`, `region_type_resource_effects.sql`).
    With the raw templater LT12 turns an empty file into a single newline and
    back on alternate runs, while `end-of-file-fixer` empties it again, so both
    hooks failed on every run. `end-of-file-fixer` already owns final
    newlines.
- `templater = raw`: these are plain SQL files, and the default Jinja templater
  would interpret `{{`, `{%`, or `{#` if help text ever contained them (none
  does today).
- `disable_noqa = True`: without it a `-- noqa` comment hides both parse errors
  and formatting (verified), and no tracked SQL file uses one.
- The upstream `sqlfluff-fix` hook runs `sqlfluff fix`, which applies every
  lint rule; the hook entry is overridden to `sqlfluff format`.
- The 18 frozen files are listed in `.sqlfluffignore`, which sqlfluff honours
  for explicitly passed paths, so the hook skips them and exits 0. Why each
  does not parse under the 4.3.0 MariaDB dialect: custom `DELIMITER //` or
  `$$` procedure blocks (5 files), `CREATE VIEW IF NOT EXISTS` (2), `BINARY`
  comparisons (8), `SHOW INDEXES` (1), the mysql client's `SOURCE` command (1),
  and an expression default, `DEFAULT (CURRENT_TIMESTAMP + INTERVAL 1 HOUR)`,
  in `sql/master_schema.sql`.
- `scripts/character-rename/test_character_rename_static.sh` looks for the
  fixed string `ALTER TABLE $table ENGINE=InnoDB`. The formatted file reads
  `ALTER TABLE player_mail ENGINE = InnoDB` (same for the other two tables, one
  line each), so the expected string changes in the SQL commit. The other text
  checks against SQL, in `scripts/test_background_help_entries.sh` and
  `scripts/events/test_pubsub_retirement.sh`, still match.
- sqlfluff prints `FAIL` with an LT02 note for the multi-table `UPDATE ... JOIN`
  in `sql/components/vessels_harbor_sandbox.sql`. The finding is not fixable,
  the file is otherwise formatted, and the hook exits 0; it is lint noise, not
  a formatting failure.
- Proof: in all 106 changed files only whitespace differs. Token streams
  (strings, identifiers, operators, and line comments kept as line-bounded
  tokens) are identical, no string literal changed, and every standalone
  `DELIMITER` line is intact.

### SQL enforcement for every future file

- The hook alone already stops the direct cases, all verified on the formatted
  tree: a new SQL file that does not parse fails the hook (exit 1), a
  `-- noqa` comment does not change that, and an unformatted new file is
  rewritten and stops the commit.
- Probing sqlfluff 4.3.0 and pre-commit found six ways around the hook that it
  does not stop on its own: an entry added to `.sqlfluffignore`; an inline
  `-- sqlfluff:exclude_rules:...` comment, which sqlfluff honours even with
  `disable_noqa`; a nested `.sqlfluff` in a subdirectory; loosened settings in
  the root `.sqlfluff`; an `exclude` added to the hook; and a top-level `files`
  or `exclude` pattern in `.pre-commit-config.yaml`, which pre-commit applies
  to every hook (found during implementation).
- `scripts/ci/check_sql_format_policy.py` closes all six. It fails when:
  - `.sqlfluffignore` has an entry, including any pattern, that is not one of
    the 18 frozen paths written into the script (entries may be removed);
  - a `.sqlfluff` or `.sqlfluffignore` is tracked anywhere but the root, or
    `setup.cfg`, `tox.ini`, or `pyproject.toml` mentions sqlfluff;
  - the root `.sqlfluff` settings differ from the reviewed ones below;
  - a tracked `.sql` file contains an inline `sqlfluff:` comment;
  - the sqlfluff hook in `.pre-commit-config.yaml` is missing, duplicated, or
    differs from `id: sqlfluff-fix`, `name: sqlfluff format`,
    `entry: sqlfluff format --processes 0 --disable-progress-bar` (for example
    by gaining `exclude`, `files`, `types`, or `args`);
  - a tracked `.sql` file outside the frozen list does not match the top-level
    `files` pattern of `.pre-commit-config.yaml`, or matches its top-level
    `exclude` pattern.
- `--self-test` builds a compliant tree plus 10 mutated copies and requires
  every mutation to be rejected. The check runs as the always-run
  `sql-format-policy` pre-commit hook and as its own step in `quality.yml`, so
  deleting the hook entry does not remove the CI check. Growing the exemption
  list means editing the script itself, which a reviewer cannot miss.
- Trials on the formatted tree, in a scratch clone: each of the six bypasses
  made the check exit 1 with a message naming the file. With a top-level
  pattern the sqlfluff hook itself skipped the new file and exited 0, which is
  why the check needs that rule.
- What remains outside the repository: `git commit --no-verify` on one
  machine, and `master` has no branch protection or ruleset, so a failing Code
  Quality run does not block a push or a merge (see Decisions to confirm).
- Writing new SQL that parses. Each form in the right-hand column was checked
  with sqlfluff 4.3.0 and the configuration below:

| Does not parse | Write instead |
| -- | -- |
| `DELIMITER` blocks for procedures, functions, and compound triggers | create the routine from C, as `src/database/db_init.c` already does for `cleanup_orphaned_dockings` and the `bi_digitalize_linestring` trigger (a hard rule in `AGENTS.md` and `docs/systems/DATABASE_INITIALIZATION_SYSTEM.md`); a single-statement `CREATE TRIGGER ... FOR EACH ROW SET ...;` parses in SQL |
| `CREATE VIEW IF NOT EXISTS` | `CREATE OR REPLACE VIEW` |
| `WHERE BINARY tag = 'x'`, `ON BINARY a = b` | `CAST(tag AS BINARY) = 'x'` (`BINARY expr` is shorthand for that cast) |
| `SHOW INDEX FROM t` | a query on `information_schema.statistics` |
| `SOURCE other.sql` | apply each file separately |
| `DEFAULT (expression)` | a literal default, or set the value where rows are written |

Also verified to parse as written: `ADD COLUMN IF NOT EXISTS`,
`CREATE INDEX IF NOT EXISTS`, `ON DUPLICATE KEY UPDATE`, `PREPARE` and
`EXECUTE`, `CREATE EVENT IF NOT EXISTS`, `DROP PROCEDURE IF EXISTS`,
`COLLATE utf8mb4_bin`, and table options such as
`ENGINE=InnoDB DEFAULT CHARSET=utf8mb4`.

### Markdown: mdformat tuned for this repository

- How the docs are written (tracked Markdown outside the dated changelogs):
  529 task-list items, 580 `---` thematic breaks, 555 setext headings, 124
  two-space hard breaks, front matter in 5 files (agent skills and issue
  templates), and 8 `[[wiki]]` links; no GitHub alerts, footnotes, or math.
- Plugins, each for a feature the docs use:
  - `mdformat-gfm`: tables, task lists, strikethrough, and autolinks.
  - `mdformat-frontmatter`: front matter passes through untouched (0 lines
    changed in the 5 files).
  - `mdformat-simple-breaks`: without it mdformat rewrites all 550 `---`
    breaks as a line of 70 underscores.
- `.mdformat.toml`: `wrap = "keep"` (prose is never re-wrapped),
  `number = true` (lists keep 1, 2, 3 rather than all 1), and
  `[plugin.tables] compact_tables = true`. The config file produces the same
  output as the equivalent command-line flags.
- Normalizations that stay: setext headings become ATX, `*` and `+` bullets
  become `-`, two-space hard breaks become a trailing backslash (visible, and
  it survives editors that trim whitespace), indented code blocks become
  fences, and blocks get blank lines around them.
- prettier was measured as the alternative: about twice mdformat's churn on
  the same files, roughly three quarters of it table re-padding, and an
  aligned table re-pads every row whenever one cell changes.
- mdformat renders each file with markdown-it before and after and refuses to
  write if the rendering differs, so escapes and normalizations never change
  what a reader sees. It added no non-ASCII characters.
- Content prep, as its own commit before formatting:
  - `docs/deployment/INTEGRATION_EXAMPLE.md` is C source with no fence, and
    `docs/development/RESOURCE_REGENERATION_API.md` opens with a C comment
    banner. GitHub renders both as garbled prose, and mdformat would add 364
    escapes to them. Fence that code as `c`.
  - mdformat then still adds 219 escapes on 148 lines in 36 files, such as
    `DEITY_PANTHEON\_\*`, `act.\*.c`, and `HP \<= 0`. Put those code-like tokens
    in backticks so no escape is needed; prose such as `[X]` in
    `docs/guides/ultimate-mud-writing-guide.md` keeps its escape. To list the
    lines, run the hook on a scratch copy and search the diff for added
    `\*`, `\_`, `\[`, or `\<`. (Implemented: 218 escapes in 35 files on the
    tree at the time; 69 prose escapes remain.)
- `docs/previous_changelogs/` stays byte-identical as a historical record, and
  the generated `lib/WILD_KB.md` is excluded as well.
  pre-commit skips the `CLAUDE.md` and `GEMINI.md` symlinks; their target
  `AGENTS.md` is formatted.
- Pandoc renders `docs/web/guides/*.html` from
  `docs/world_game-data/OEDIT_GUIDE.md`, `MOB_FLAGS.md`, and `ROOM_FLAGS.md`.
  Pandoc Markdown needs a blank line before a list where CommonMark does not,
  so once mdformat inserts those blank lines pandoc renders real lists, and
  `wtool.py docs --check` (which runs `generate-web-guides.sh --check`) fails
  until the guides are regenerated. The Markdown commit regenerates them with
  pandoc 3.1.3, the version on the host and in CI's Ubuntu 24.04 packages.
  Future edits follow the same order: let the hook format those three files,
  then run the generator.

### YAML, JSON, HTML, CSS, and JavaScript: prettier

- One tool covers five types. The width comes from `.editorconfig`:
  `max_line_length = 100` moves from `[*.{c,h}]` into `[*]`, where only
  prettier and editors read it. Indentation is already 2.
- `pre-commit/mirrors-prettier` is archived; `rbubley/mirrors-prettier` is the
  maintained mirror. The hook is limited to
  `types_or: [yaml, json, html, css, javascript]` so it never touches Markdown
  or PHP.
- pre-commit classifies `.clang-format` and `.clang-tidy` as YAML, so prettier
  formats them too. Only quote style changes, and `clang-format --dump-config`
  and `clang-tidy --dump-config` print identical output before and after.
- `.prettierignore` (prettier honours it for explicitly passed paths, so the
  hook and editors agree):
  - `docs/architecture-maps/`: pages and sources sealed by the sha256 receipts
    in its README.
  - `lib/rol-conversion/runs/`: conversion run records.
  - `docs/web/guides/` and `docs/web/assets/pandoc-template.html`: pandoc output
    and its template; formatting either makes `generate-web-guides.sh --check`
    fail.
  - `docs/web/spells/`, `docs/spells_by_class.html`,
    `docs/spells_reference.html`: generated by `util/generate_spell_html.sh`
    and `util/generate_spell_html_detailed.py`, and invalid HTML that prettier
    cannot parse.
  - `scripts/world/wtool_constants.json`: written by
    `wtool constants sync --write`; `wtool constants sync --check` and
    `test_checked_in_manifest_is_current` fail if it is reformatted.
- Proof: `prettier --debug-check`, which requires the output to parse to the
  same AST, passes for all 44 in-scope files.
  `scripts/development/check-dg-docs.py` reads `dg-reference.js` and the DG
  pages with regular expressions and passes after formatting.

### CMake: gersemi

- `.gersemirc` sets `line_length: 100` (about 790 changed lines, against 985 at
  the default 80). Indentation stays at gersemi's default of 4, which the file
  already uses.
- `scripts/ci/check_build_parity.py` expects `add_executable(cutest` on one
  line. gersemi moves the target name to the next line, so
  `parse_cmake_executable` finds nothing. Change its pattern from
  `r"^\s*add_executable\(" + re.escape(target)` to
  `r"^\s*add_executable\(\s*" + re.escape(target)`. The new pattern returns the
  same tokens on the old and the new file, and `parse_cmake_lists` output is
  identical without any change.
- Proof: `check_build_parity.py` passes on the formatted tree; the CMake jobs
  in `test.yml` configure and build it.

### PHP: php-cs-fixer

- The 7 files are the web tools in `util/`; 6 of them mix PHP with inline
  HTML.
- `@prettier/plugin-php` 0.25.0 would need no PHP runtime, but it broke 3 of
  the 7 files (`php -l` failed and the compiled opcodes changed) and fails
  prettier's own `--debug-check`. Rejected.
- php-cs-fixer 3.95.25 with `@PER-CS3x0`: PER Coding Style 3.0, PHP-FIG's
  maintained successor to PSR-12, named by version so a php-cs-fixer upgrade
  cannot change the style silently. `@PSR12` would change fewer lines (386
  against 528) but is frozen, and the unversioned `@PER-CS` floats with
  releases.
- Proof: `php -l` passes for all 7 formatted files, the opcodes PHP 8.3
  compiles from each file (an opcache debug dump with file paths removed) are
  identical before and after, and a second pass finds nothing to fix. The
  output is byte-identical between the `php:8.3-cli` image and Ubuntu 24.04's
  `php8.3-cli` (PHP 8.3.6).
- Hook: a local `language: system` hook running
  `scripts/development/format_php.sh`. The wrapper downloads the pinned phar
  once into
  `${LUMINARI_FORMATTER_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/luminari-formatters}`,
  verifies its sha256
  (`80cad475fc5112fdbfab8bd66e51665ed78c1b849b918dab81fb63b7a7003b41`, the
  digest GitHub publishes for that release asset), and runs
  `php <phar> fix --config=.php-cs-fixer.dist.php --using-cache=no --quiet -- <files>`.
  php-cs-fixer needs `--config` whenever it gets more than one path. A
  committed phar would trip the 500 KB large-file hook, and composer would add
  a PHP dependency tree for one tool.

### PowerShell: PSScriptAnalyzer formatter

- The 5 scripts are `lib/world/backup-zone.ps1`, `lib/world/validate-zone.ps1`,
  and three in `util/powershell/`; they use 2-space indentation and braces on
  the same line, and `util/powershell/README_powershell.md` also runs them
  under Windows PowerShell. Formatting changes no syntax.
- `PSScriptAnalyzerSettings.psd1` at the root (the file name PowerShell editor
  tooling looks for) holds PSScriptAnalyzer's `CodeFormattingOTBS` preset with
  `IndentationSize = 2`, without the Microsoft signature block the shipped
  preset carries. It changes 5 files, +18/-18: `Param(` becomes `param(`,
  commas get a following space, and column-aligned `=` loses its padding. The
  same preset at 4 spaces changes 306 lines, and the Allman preset 410.
- Proof: for all 5 files the PowerShell parser's tokens are identical before
  and after, compared case-insensitively outside strings and comments
  (PowerShell names are case-insensitive, and `PSUseCorrectCasing` fixes
  keyword and cmdlet case). A second pass changes nothing, and LF endings and
  the final newline are kept.
- Hook: a local `language: system` hook running
  `pwsh -NoProfile -NonInteractive -File scripts/development/format_powershell.ps1 <files>`.
  The wrapper saves PSScriptAnalyzer 1.25.0 once into the same cache with
  `Save-PSResource`, formats with
  `Invoke-Formatter -Settings PSScriptAnalyzerSettings.psd1`, and rewrites only
  files that changed, as UTF-8 without a byte-order mark.

### Runtimes for PHP and PowerShell

| Where | PHP | PowerShell |
| -- | -- | -- |
| Development host (Ubuntu 24.04, WSL2) | `sudo apt-get install -y php8.3-cli` | Microsoft's package repository, then `sudo apt-get install -y powershell` (commands in Step 0) |
| GitHub `ubuntu-latest` runner (24.04) | PHP 8.3.6 preinstalled | PowerShell 7.6.5 and PSScriptAnalyzer 1.25.0 preinstalled |
| Local CI image, `scripts/ci/local/Dockerfile` | add `php8.3-cli` to the apt list | add the Microsoft repository and `powershell` |

Both host recipes ran cleanly in a fresh `ubuntu:24.04` container (PHP 8.3.6,
PowerShell 7.6.6). Contributors who never stage PHP or PowerShell files do not
need either runtime; `pre-commit run --all-files` does.

### Not formatted

- World files, `lib/text/help/help.hlp`, JSON and JSONL run records, the
  architecture-map receipts, and the legal archive: generated or byte-sealed.
- Makefiles, Dockerfiles, and `.gitleaks.toml`: no mature formatter;
  `.editorconfig` and the hygiene hooks cover whitespace, newlines, and tabs.

## Configuration to add

`.pre-commit-config.yaml`: this block goes after the clang-format repository
and before "General file hygiene".

```yaml
  # Python: ruff's formatter. The upstream hook also accepts Markdown, whose
  # embedded code blocks belong to mdformat, so it is limited to Python.
  - repo: https://github.com/astral-sh/ruff-pre-commit
    rev: v0.16.7
    hooks:
      - id: ruff-format
        types_or: [python]

  # Shell: shfmt takes its settings from .editorconfig.
  - repo: https://github.com/scop/pre-commit-shfmt
    rev: v3.14.1-1
    hooks:
      - id: shfmt

  # SQL: layout-only `sqlfluff format`; the upstream hook's `fix` would apply
  # every lint rule. scripts/ci/check_sql_format_policy.py pins this hook.
  - repo: https://github.com/sqlfluff/sqlfluff
    rev: 4.3.0
    hooks:
      - id: sqlfluff-fix
        name: sqlfluff format
        entry: sqlfluff format --processes 0 --disable-progress-bar

  # Markdown: dated changelogs stay byte-identical historical records, and
  # lib/WILD_KB.md is written by the wilderness knowledge-base command.
  - repo: https://github.com/executablebooks/mdformat
    rev: 1.0.0
    hooks:
      - id: mdformat
        additional_dependencies:
          - mdformat-gfm==1.0.0
          - mdformat-frontmatter==2.1.2
          - mdformat-simple-breaks==0.1.0
        exclude: ^(docs/previous_changelogs/|lib/WILD_KB\.md$)

  # YAML, JSON, HTML, CSS, and JavaScript; generated and sealed files are in
  # .prettierignore.
  - repo: https://github.com/rbubley/mirrors-prettier
    rev: v3.9.6
    hooks:
      - id: prettier
        types_or: [yaml, json, html, css, javascript]

  # CMake
  - repo: https://github.com/BlankSpruce/gersemi-pre-commit
    rev: 0.29.1
    hooks:
      - id: gersemi
```

These hooks join the existing `repo: local` block, before the pre-push
`check-build` hook:

```yaml
      # PHP: pinned php-cs-fixer with .php-cs-fixer.dist.php; needs php on PATH.
      - id: php-cs-fixer
        name: php-cs-fixer
        entry: scripts/development/format_php.sh
        language: system
        types: [php]
        require_serial: true
      # PowerShell: PSScriptAnalyzer formatter with PSScriptAnalyzerSettings.psd1;
      # needs pwsh on PATH.
      - id: powershell-format
        name: PSScriptAnalyzer format
        entry: pwsh -NoProfile -NonInteractive -File scripts/development/format_powershell.ps1
        language: system
        types: [powershell]
        require_serial: true
      # Every future SQL file stays under the sqlfluff hook.
      - id: sql-format-policy
        name: SQL format policy
        entry: python scripts/ci/check_sql_format_policy.py
        language: python
        additional_dependencies: [pyyaml==6.0.2]
        pass_filenames: false
        always_run: true
```

`require_serial` keeps the two wrappers from racing to fill the tool cache on
first use.

`ruff.toml`:

```toml
line-length = 100
```

`.sqlfluff`:

```ini
[sqlfluff]
dialect = mariadb
templater = raw
# Inline noqa comments cannot suppress formatting or parse errors.
disable_noqa = True
large_file_skip_byte_limit = 0
max_line_length = 100
# Layout only: capitalisation rules rename case-sensitive table identifiers,
# LT05 (line length) cannot always be fixed automatically, and end-of-file-fixer
# owns final newlines (LT12 flips empty files between 0 and 1 byte).
exclude_rules = LT05, LT12, CP01, CP02, CP03, CP04, CP05

[sqlfluff:indentation]
tab_space_size = 2
```

`.sqlfluffignore`, headed by a comment that the list is frozen and enforced by
`scripts/ci/check_sql_format_policy.py`:

```text
lib/pubsub_v3_schema.sql
sql/components/ai_region_hints_schema.sql
sql/components/ai_service_migration.sql
sql/components/dynamic_descriptions_deployment.sql
sql/components/help_gdb_binary_path.sql
sql/components/help_race_yuan_ti_entries.sql
sql/components/help_rol_feat_entries.sql
sql/components/help_rol_player_kits.sql
sql/components/help_system_indexes.sql
sql/components/help_vessel_entries.sql
sql/components/narrative_weaver_installation.sql
sql/components/pubsub_v3_schema.sql
sql/components/verify_help_rol_feat_entries.sql
sql/components/verify_help_rol_player_kits.sql
sql/components/verify_help_specproc_entries.sql
sql/components/verify_help_vessel_entries.sql
sql/components/vessels_phase2_schema.sql
sql/master_schema.sql
```

`.mdformat.toml`:

```toml
wrap = "keep"
number = true

[plugin.tables]
compact_tables = true
```

`.prettierignore`: the eight entries listed under prettier, each with a
one-line reason comment.

`.gersemirc`:

```yaml
line_length: 100
```

`.php-cs-fixer.dist.php`:

```php
<?php

// Formatting for the PHP tools under util/: PER Coding Style 3.0, named by
// version so a php-cs-fixer upgrade cannot change the style silently.
return (new PhpCsFixer\Config())
    ->setRules(['@PER-CS3x0' => true])
    ->setFinder(PhpCsFixer\Finder::create()->in(__DIR__ . '/util'));
```

`PSScriptAnalyzerSettings.psd1`: PSScriptAnalyzer 1.25.0's
`Settings/CodeFormattingOTBS.psd1` with `IndentationSize = 2` and without its
signature block, headed by a comment naming the preset. It enables
`PSPlaceOpenBrace`, `PSPlaceCloseBrace`, `PSUseConsistentWhitespace`,
`PSUseConsistentIndentation`, `PSAlignAssignmentStatement`, and
`PSUseCorrectCasing` with the preset's options.

`scripts/development/format_php.sh`, `scripts/development/format_powershell.ps1`,
and `scripts/ci/check_sql_format_policy.py`: behavior as described in the PHP,
PowerShell, and SQL enforcement sections. Each was prototyped and exercised in
the dry run.

`.editorconfig`: add `max_line_length = 100` to `[*]` and delete the
`[*.{c,h}]` section; add a `[*.sh]` section with `switch_case_indent = true`;
keep `[*.py] indent_size = 4`; update the header comment to say that shfmt and
prettier read this file.

## Required supporting changes

1. `scripts/events/test_pubsub_retirement.sh`: the glob rewrite (shell commit).
2. `scripts/character-rename/test_character_rename_static.sh`: expect
   `ALTER TABLE $table ENGINE = InnoDB` (SQL commit).
3. Markdown content prep: fence the two C documents and put code-like tokens
   in backticks (its own commit, before the Markdown commit).
4. `docs/web/guides/*.html`: regenerate with
   `scripts/development/generate-web-guides.sh` (Markdown commit).
5. `scripts/ci/check_build_parity.py`: the `add_executable\(\s*` pattern
   (CMake commit).
6. `.github/workflows/quality.yml`: drop both `paths` filters, because the
   formatters now cover most of the tree, and keep the branch filters. Steps:
   print `php --version` and `pwsh --version` first so a runner image change
   fails loudly; run `pre-commit run --all-files --show-diff-on-failure` with
   an error message naming that command; then run
   `python3 scripts/ci/check_sql_format_policy.py --self-test` and
   `python3 scripts/ci/check_sql_format_policy.py`. The `README.md` badge keeps
   working because the workflow file name is unchanged.
7. `scripts/ci/local/Dockerfile`: add `php8.3-cli` to the apt list, add the
   Microsoft repository and `powershell`, and set
   `ENV LUMINARI_FORMATTER_CACHE=/tmp/luminari-formatters` so the wrappers
   have a writable cache as uid 1000. Rebuild the image, which preinstalls the
   hook environments.
8. Documentation:
   - `AGENTS.md` (also read as `CLAUDE.md` and `GEMINI.md`): one Conventions
     line naming the formatter for each type; that bulk formatting goes through
     `pre-commit run <hook-id> --all-files`; and a rule that new SQL must pass
     the sqlfluff hook, with no `.sqlfluffignore` entries, inline `sqlfluff:`
     comments, or extra sqlfluff configuration. The rule that stored routines
     and multi-statement triggers live in `src/database/db_init.c`, not in
     `.sql` files, is already in its Critical Rules.
   - `CONTRIBUTING.md` step 3: replace "Format only the files you changed with
     the repository `.clang-format`" with installing the hooks
     (`pre-commit install`) and letting them format staged files, noting the
     PHP and PowerShell runtimes.
   - `docs/guides/SETUP_AND_BUILD_GUIDE.md`: a Formatting section after Source
     Tree Hygiene with the tool table, runtimes and install commands, the config
     files, each exclusion and its reason, the table of SQL forms that parse,
     the guide regeneration order, and the upgrade procedure below. Correct
     "HTML under `docs/` is generated web output": only the pandoc guides and
     the spell pages are generated.
   - `docs/guides/TESTING_GUIDE.md` already says to rebuild the local CI image
     after hook changes; add that the image now carries PHP and PowerShell.

## Implementation sequence

### Step 0: runtimes, timing, and open work

- Install the runtimes on the development host (the same commands ran cleanly
  in a fresh `ubuntu:24.04` container):

  ```bash
  sudo apt-get install -y php8.3-cli
  wget -q https://packages.microsoft.com/config/ubuntu/24.04/packages-microsoft-prod.deb
  sudo dpkg -i packages-microsoft-prod.deb && rm packages-microsoft-prod.deb
  sudo apt-get update && sudo apt-get install -y powershell
  ```

- Land it while few branches are open. Today that is PR #190 (5 files in
  scope) and the local unmerged branches `ai-secret-lifecycle-99`,
  `arch-n-worktree`, and `feat-dev`, plus backup branches.

- Fetch first: other sessions push to `master`. Do not commit while a
  background build or test reads the tree, because the hook stashes unstaged
  files while it runs.

- Recipe for a branch that is open when this lands: check out the new config
  and wrapper files from `origin/master` (`.pre-commit-config.yaml`,
  `.editorconfig`, `ruff.toml`, `.sqlfluff`, `.sqlfluffignore`,
  `.mdformat.toml`, `.prettierignore`, `.gersemirc`, `.php-cs-fixer.dist.php`,
  `PSScriptAnalyzerSettings.psd1`, and the three scripts), run
  `pre-commit run --files $(git diff --name-only origin/master...HEAD)`, commit
  the result, then rebase. What still conflicts is the branch's real edits.

### Steps 1 to 8: one commit per formatter

For each step: add the config and the hook entry, run
`pre-commit run <hook-id> --all-files` (it exits 1 after rewriting files),
make the supporting change, stage, rerun until the hook passes, run the step's
checks, and commit. The Markdown content prep (supporting change 3) is its own
commit immediately before step 4.

| Step | Commit | Hook id | Also in the commit | Checks |
| -- | -- | -- | -- | -- |
| 1 | Python | `ruff-format` | `ruff.toml` | AST comparison; `python -m unittest discover -s scripts/world/tests -t scripts/world -v`; `python scripts/world/wtool.py constants sync --check` |
| 2 | Shell | `shfmt` | glob rewrite; `[*.sh]` section | `shfmt --to-json` AST comparison; `bash -n` on every script; `git diff --summary` shows no mode changes |
| 3 | SQL | `sqlfluff-fix`, `sql-format-policy` | `.sqlfluff`; `.sqlfluffignore`; the policy check; rename test string | token-stream comparison; `check_sql_format_policy.py --self-test` and a plain run; `make test-character-rename-static`; `scripts/test_background_help_entries.sh`; `scripts/events/test_pubsub_retirement.sh` |
| 4 | Markdown | `mdformat` | `.mdformat.toml`; regenerated guides | `python scripts/world/wtool.py docs --check`; `python3 scripts/ci/check_source_hygiene.py`; no added escapes outside prose |
| 5 | Web and config data | `prettier` | `.prettierignore`; `[*]` width | `prettier --debug-check` on the in-scope files; `python3 scripts/development/check-dg-docs.py`; `wtool.py constants sync --check`; unchanged `clang-format --dump-config` and `clang-tidy --dump-config` |
| 6 | CMake | `gersemi` | `.gersemirc`; parity pattern | `python3 scripts/ci/check_build_parity.py`; `gersemi --safe --diff` on the unformatted files once (its equivalence check is off by default); CMake configure and build |
| 7 | PHP | `php-cs-fixer` | `.php-cs-fixer.dist.php`; `format_php.sh` | `php -l` on each file; opcode comparison; a second run finds nothing |
| 8 | PowerShell | `powershell-format` | `PSScriptAnalyzerSettings.psd1`; `format_powershell.ps1` | token comparison; a second run changes nothing |

The AST, token, and opcode comparisons are one-off scripts run from a scratch
directory, not committed:

- Python: `ast.dump` output with docstrings normalized.
- Shell: `shfmt --to-json` output with position objects removed.
- SQL: token lists after dropping whitespace, with string literals and line
  comments as single tokens.
- PHP: `php -d zend_extension=opcache -d opcache.enable_cli=1 -d opcache.opt_debug_level=0x10000 -r 'opcache_compile_file($argv[1]);'`
  output with file paths removed.
- PowerShell: `[System.Management.Automation.Language.Parser]::ParseInput`
  tokens without newlines, lowercased outside strings and comments.

### Step 9: CI, local CI image, and documentation

Supporting changes 6 to 8. The plan itself is deleted in Step 10, once
`.git-blame-ignore-revs` can list the merged commits; its lasting content is
in the setup guide, as `docs/ongoing-projects/README.md` asks.

### Step 10: after merge

Merge with "Rebase and merge" (enabled on the repository) so the formatting
commits keep their own SHAs on `master`, then add `.git-blame-ignore-revs`
listing the eight formatter commits (not the content prep commit). GitHub
applies the file automatically; locally, run
`git config blame.ignoreRevsFile .git-blame-ignore-revs`. The one-line
supporting changes inside those commits stay there, because each depends on
the formatting it accompanies.

### Final verification

- `pre-commit run --all-files` passes twice in a row on the final tree.
- Local CI per `docs/guides/TESTING_GUIDE.md` on the final commit, after
  rebuilding the image: the `quality`, `hygiene`, `test`, and `integration`
  jobs. `integration.yml` loads `sql/master_schema.sql` and every SQL
  component into MariaDB 10.11. Iterate on the host; run the container matrix
  once.

## Dry-run record

A scratch clone of `570193508` was taken to the planned end state with the
real hooks: every config and wrapper, the supporting changes, the fences for
the two C documents, bulk formatting through `pre-commit run <hook-id> --all-files` for each formatter, and regenerated guides. PHP ran through a
`php` shim into the `php:8.3-cli` image and PowerShell from the 7.6.6 archive.

- Each formatter hook passed on its second run.
- `pre-commit run --all-files` passed on the formatted tree, including
  `php-cs-fixer`, `PSScriptAnalyzer format`, and `SQL format policy`; a warm
  rerun over every file took 12 s on the 16-core host, with PHP going through
  its Docker shim.
- An earlier run exposed the LT12 and empty-file conflict; with LT12 excluded
  it is gone.
- Passed: `check_build_parity.py`, `check_source_hygiene.py`,
  `check-dg-docs.py`, `wtool.py constants sync --check`,
  `wtool.py docs --check`, `test_background_help_entries.sh`,
  `test_character_rename_static.sh`, `test_pubsub_retirement.sh`, `bash -n` on
  every script, and `check_sql_format_policy.py` with and without
  `--self-test`.
- World-tool unit tests: 537 ran with identical results before and after
  formatting. The clone had no configured build, so 27 C-compiling tests
  errored the same way in both runs; CI's configured job runs them.
- SQL enforcement trials: the five bypasses each failed the policy check; a new
  unparsable file failed the sqlfluff hook with and without `-- noqa`; a new
  unformatted file was rewritten and failed the commit.

## Ablation record

Removed or simplified during planning:

- sql-formatter: breaks `DELIMITER` lines.
- prettier for Markdown: twice mdformat's churn, mostly table re-padding.
- `@prettier/plugin-php`: no runtime needed, but it broke 3 of 7 PHP files.
- sqlfluff capitalisation rules, LT12, and the upstream `sqlfluff fix` hook:
  they rename identifiers, fight `end-of-file-fixer`, and apply lint fixes;
  formatting is the scope.
- A sqlfluff lint hook: formatting is the scope, and the one unfixable layout
  note (`vessels_harbor_sandbox.sql`) does not fail the formatter.
- Scanning SQL for `noqa`: `disable_noqa = True` neutralizes it.
- A separate baseline file for the SQL exemptions: the frozen list lives in the
  check itself, so growth is a visible code change.
- One CI job per tool, and hook-environment caching: one
  `pre-commit run --all-files` step covers every hook.
- A committed php-cs-fixer phar, a composer tree, or a Docker-based hook: the
  wrapper's pinned, checksummed download needs none of them.
- Prefetching the PHP and PowerShell tools into the local CI image: the
  wrappers fetch them on first use; the image only needs the runtimes and a
  writable cache path.
- TOML, Makefile, and Dockerfile formatters: no mature tool.
- A `.prettierrc` and shfmt flags: `.editorconfig` already carries width and
  indentation, so editors and hooks read one file.
- A MariaDB dump comparison for SQL: identical token streams already prove the
  statements unchanged, and `integration.yml` applies every component.
- A whitespace-tolerant assertion helper for the rename test: one updated
  expected string is enough, because the hook now keeps that spacing
  canonical.
- Backticks for the 28 escaped prose brackets: the escape is correct there.

Kept: each supporting change fixes a failure reproduced in a dry run, each
ignore entry names a parser gap, a generator, or a hash seal, and each policy
rule closes a bypass that was demonstrated against sqlfluff 4.3.0.

## Decisions to confirm

- Server-side enforcement: add a `master` ruleset that requires the Code
  Quality check. Without it the hook and CI report violations but cannot block
  a push made with `--no-verify` or a merge over a red check.

## Out of scope

Linters and lint autofixes (clang-tidy, `ruff check`, shellcheck,
`sqlfluff lint`, yamllint, markdownlint, PSScriptAnalyzer rules other than
formatting), rewriting the 18 frozen SQL files, fixing the invalid HTML the
spell page generators emit, and bumping the existing clang-format and
pre-commit-hooks pins.

## Upgrades

`pre-commit autoupdate` bumps the `rev` pins; the mdformat plugin versions in
`additional_dependencies` and the PyYAML pin are bumped by hand. php-cs-fixer
is bumped by changing the version and sha256 in `format_php.sh`, and
PSScriptAnalyzer by changing the version in `format_powershell.ps1`. After any
bump, run `pre-commit run --all-files`, commit the result as a
formatting-only commit, and add it to `.git-blame-ignore-revs`. After a
sqlfluff bump, also run it against the `.sqlfluffignore` entries and remove
any that now parse.
