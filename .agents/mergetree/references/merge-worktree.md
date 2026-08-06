# LuminariMUD Worktree Merge Workflow

## Contents

1. Mission and invariants
2. Reconnaissance
3. Approval plan
4. Merge execution
5. Conflict resolution
6. Verification and handoff

## 1. Mission and invariants

Merge one worktree's feature branch into the canonical default-branch worktree, normally
`master` in this repository. Keep the result local, minimal, reviewable, buildable, and tested.
Use the feature intent as the tiebreaker while preserving compatible target-branch behavior.

Obtain or establish these facts before proceeding: canonical worktree path, source worktree
path, target branch and ref, source branch and ref, and a one-to-three-sentence feature intent.
Discover paths and refs from Git rather than guessing. If the source is detached, identify its
exact commit and ask the user to confirm it instead of inventing a branch. If feature intent was
not supplied, derive a candidate from the commits and diff, then ask the user to confirm it before
planning the merge.

Enforce these invariants:

- Read the applicable `AGENTS.md` first. Recheck it if work crosses into a nested scope.
- Inspect only `APP_ENV` in `lib/.env` without printing the file or any credentials. Stop on a
  production environment; do not modify code or create refs or worktrees there.
- Never push, delete a branch or worktree, rebase the target, rewrite history, force-update a
  ref, bypass hooks, or run `git reset --hard`, `git clean -fd`, or force-push variants.
- Do not run `git merge --abort` merely because conflicts exist. Ask before abandoning work.
- Modify only conflicts and files directly required for semantic integration, compilation,
  tests, documentation, or help. Do not hide failures with stubs, TODOs, or disabled tests.
- Never modify `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`; use the corresponding
  example header when a template change is genuinely required. Never modify `lib/.env` or
  `lib/mysql_config` without explicit permission; prefer their example files.
- Do not hand-merge generated output. Resolve its inputs, determine the repository's generator,
  and regenerate. `unittests/CuTest/AllTests.c` is generated from `Test*` functions.
- Preserve unrelated user changes. A dirty canonical worktree blocks the merge; a dirty source
  worktree means its uncommitted changes are not mergeable and must be resolved with the user.
- Keep documentation ASCII, UTF-8, and LF. Do not add AI attribution anywhere, including commits.

If a risky action seems necessary, stop and explain the need instead of running it.

## 2. Reconnaissance

Perform read-only reconnaissance before editing or merging:

1. Locate worktrees and refs with `git worktree list --porcelain`, `git rev-parse --show-toplevel`,
   `git branch --show-current`, and `git status --short --branch` in both worktrees. Confirm the
   canonical worktree actually has the target branch checked out. Detect the default branch from
   `refs/remotes/origin/HEAD`; adapt safely if it differs from the repository's usual `master`.
2. Stop on a dirty canonical worktree. Report a dirty source worktree and stop unless the user
   first makes its intended changes part of the source ref.
3. Record `git log -5 --oneline --decorate` for both refs. Inspect their merge base, ahead/behind
   relationship, and `git diff --name-status`, `--stat`, and `--check` for `target...source`.
   Summarize; do not paste large logs. Stop if no merge base exists. If the source has no unique
   commits, report that it is already integrated instead of manufacturing a merge commit.
4. Trace every changed subsystem from definitions, includes, call sites, build manifests, tests,
   help files, and documentation. Do not infer behavior from filenames. Resolve shared/core code
   before dependent feature directories. Remember that files under `src/` use one feature-directory
   level and cross-directory headers require path-qualified includes.
5. If a source file was added, removed, or moved, verify both `Makefile.am` and `CMakeLists.txt`.
   If gameplay or commands changed, verify the relevant `lib/text/help/` entries and docs.
6. Determine remote freshness from available tracking refs. If a fetch is needed to know current
   state, say so first. If the target is behind, propose the exact non-destructive update command,
   normally `git pull --ff-only`, and do not run it without explicit approval.

After confirming development, clean worktrees, and the exact target HEAD, create a unique,
non-overwriting safety ref such as `backup/pre-merge-YYYY-MM-DD-HHMMSS`. Point it at target HEAD
with `git branch <safety-ref> HEAD`, then report its exact name and commit. Never use `-f`.

## 3. Approval plan

Before `git merge` or any file edit, present a concrete plan and wait for explicit approval.
Include:

- Canonical path, source path, exact refs and commits, feature intent, and safety ref.
- Any approved target update, followed by `git merge --no-commit --no-ff <source-ref>`.
- Likely conflict areas and a shared-code-before-consumers resolution order.
- The smallest expected edit scope and targeted checks for affected subsystems.
- Exact final verification commands and the intended merge commit message.

Do not treat approval of reconnaissance as approval to merge.

## 4. Merge execution

After approval, return to the canonical worktree and recheck `pwd`, environment, branch, HEAD,
and clean status. Recheck that the source ref still names the reviewed commit. Run only the approved
fast-forward-only target update, if any.

If an approved target update changed HEAD, preserve the original safety ref and create a second
unique safety ref at the updated HEAD. Never move or overwrite the original ref.

Run `git merge --no-commit --no-ff <source-ref>`. Do not commit yet. If it merges cleanly, review
the staged result and proceed to verification. If it conflicts, retain the merge state and use the
structured process below.

## 5. Conflict resolution

List unresolved paths with `git diff --name-only --diff-filter=U` and choose an explicit order:
build contracts and shared headers, core implementation, feature subsystems, tests, then docs/help.

For each path:

1. Inspect the base, target (`ours`), source (`theirs`), surrounding code, relevant commits, and
   callers. Summarize what each side does before choosing a resolution.
2. Combine compatible changes. Preserve public contracts and target behavior while retaining the
   feature intent. Prefer the more complete implementation only after integrating required behavior
   from the other side. Never choose an entire side solely to clear markers.
3. Make the smallest semantic edit. Follow GNU C23 and existing style: two-space indentation,
   declarations at block tops, `/* */` comments, no variable-length arrays, safe strings, and no
   mechanical restyling of legacy code.
4. Reopen the result; trace expected symbols, types, registration tables, and consumers; run the
   narrowest useful check; then stage only that resolved path. CuTest has no per-function filter,
   so use the appropriate suite or focused harness rather than inventing one.

For generated conflicts, resolve sources and manifests, remove conflict stages through regeneration,
then review the generated diff. For source-layout changes, keep `Makefile.am` and `CMakeLists.txt`
synchronized and preserve explicit cross-subsystem include paths.

If semantics remain ambiguous after examining intent, history, base, callers, and tests, stop and
present the concrete alternatives and tradeoffs. Do not guess. Also stop if failures imply a broad
redesign or production/configuration change outside the approved merge scope.

Before verification, require an empty unresolved-path list and use `git diff --check` plus a scan of
changed text files for conflict markers. Review both the staged diff and merge status.

At roughly 60 percent of available context, if conflicts or substantial validation remain, do not
commit. Report the safety refs, target and source commits, resolved and unresolved paths, decisions
made, remaining ambiguities, commands run, and test results so another context can continue without
reconstructing the merge.

## 6. Verification and handoff

Choose targeted tests from the changed subsystem, then run the repository's authoritative checks.
At minimum, unless the user approves a narrower exception:

```bash
make clean
make -j$(nproc)
make test-all
```

`make test-all` runs the production-linked CuTest suite, world tools, protocol parser, character
rename checks, and `make install`; the install also removes the root-level `circle` artifact. Use
`autoreconf -fvi && ./configure` only when configuration files are absent or inputs require it.
MariaDB is required. Save complete test output outside the repository when tool output may truncate,
and report the command, exit status, and relevant failure details without dumping huge logs.

After tests pass:

1. Review `git diff --cached --check`, the full staged diff, and its stat as if reviewing a PR.
   Reject accidental scope, API loss, duplicated registration, stale build lists, disabled tests,
   credential changes, placeholders, and unrelated formatting.
2. Commit the merge with a clear human-authored message describing the feature and notable
   reconciliation. Do not use `--no-verify`. If a hook changes files, review those changes, rerun
   affected checks, stage deliberately, and retry only if the hook rejected the commit. If the
   commit succeeded but left changes, stop before creating another commit. Never silently amend or
   bypass the hook.
3. Inspect `git diff --stat HEAD^1..HEAD`, `git diff --check HEAD^1..HEAD`, the merge commit
   summary, and final `git status --short --branch`. Require a clean worktree and no unresolved
   markers.
4. Report affected subsystems, notable resolutions, tests and exact results, assumptions, safety
   ref, merge commit, and any focused follow-up review. Flag anything the second-pass review would
   reject.

Stop for user review. Do not push, remove the safety ref, or delete the source branch/worktree.
