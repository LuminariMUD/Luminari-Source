# LuminariMUD Worktree Merge Workflow

## Contents

1. Mission and invariants
2. Autonomous preflight and publication
3. Reconnaissance and execution plan
4. Merge execution
5. Conflict resolution
6. Verification, publication, and handoff

## 1. Mission and invariants

Merge one worktree's feature branch into the canonical default-branch worktree, normally
`master` in this repository. Keep the result minimal, reviewable, buildable, tested, committed,
and published. Use the feature intent as the tiebreaker while preserving compatible target-branch
behavior.

Operate autonomously once the user invokes this skill. Report the plan as a progress update, then
continue without waiting for routine approval. Stop only for production state, protected or secret
material, destructive history changes, unresolved intent or semantics, unsafe remote divergence,
or failures that require a broader design decision.

Obtain or establish these facts before proceeding: canonical worktree path, source worktree
path, target branch and ref, source branch and ref, and a one-to-three-sentence feature intent.
Discover paths and refs from Git rather than guessing. If the source is detached, identify its
exact commit and whether a remote ref contains it. Derive missing feature intent from commits,
diffs, tests, docs, and call sites. Proceed when that evidence is coherent; stop only when intent
remains materially ambiguous after tracing it.

Enforce these invariants:

- Read the applicable `AGENTS.md` first. Recheck it if work crosses into a nested scope.
- Inspect only `APP_ENV` in `lib/.env` without printing the file or any credentials. Stop on a
  production environment; do not modify code or create refs or worktrees there.
- Never delete a branch or worktree, rebase the target, rewrite history, force-update a ref,
  bypass hooks, run `git reset --hard` or `git clean -fd`, or use force-push variants. Ordinary
  non-force fetches, pulls, and pushes for the exact source and target refs are part of this
  workflow.
- Do not run `git merge --abort` merely because conflicts exist. Leave the merge state intact and
  stop only when the selected refs or operation are proven unsafe or incorrect.
- Modify only conflicts and files directly required for semantic integration, compilation,
  tests, documentation, or help. Do not hide failures with stubs, TODOs, or disabled tests.
- Never modify `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`; use the corresponding
  example header when a template change is genuinely required. Never modify `lib/.env` or
  `lib/mysql_config` without explicit permission; prefer their example files.
- Do not hand-merge generated output. Resolve its inputs, determine the repository's generator,
  and regenerate. `unittests/CuTest/AllTests.c` is generated from `Test*` functions.
- Preserve unrelated user changes. Classify and resolve dirty state autonomously through the
  preflight below; never fold unrelated work into the merge commit or discard uncertain changes.
- Keep documentation ASCII, UTF-8, and LF. Do not add AI attribution anywhere, including commits.

If a risky action seems necessary, stop and explain the need instead of running it.

## 2. Autonomous preflight and publication

Make cleanliness and publication the first gate, before deep feature analysis:

1. Locate worktrees and refs with `git worktree list --porcelain`, `git rev-parse --show-toplevel`,
   `git branch --show-current`, and `git status --short --branch` in both worktrees. Confirm the
   canonical worktree has the detected default branch checked out.
2. Fetch the configured remote to refresh tracking refs. A fetch is non-destructive; announce it as
   progress but do not wait for approval. If it fails, retry only when the failure is transient.
   Stop when publication cannot be verified.
3. Inspect every tracked and untracked dirty path before acting. Trace its diff, history, ignore
   rules, and relationship to the branch intent.
   - For coherent intended work, run proportional checks, create a separate human-authored commit
     on that branch, and publish it before continuing.
   - For mixed but separable work, stage and commit coherent groups separately. Never hide unrelated
     work inside the feature merge.
   - For provably incidental generated state, restore the exact tracked file or remove the
     untracked artifact. Add an appropriate ignore rule in a separate target cleanup when recurrence
     is likely.
   - Stop only when ownership or intent remains ambiguous, a secret or protected file is involved,
     or safe partitioning is impossible. Never guess, discard uncertain work, or publish credentials.
4. Require both worktrees to be clean after preflight. Determine each branch's upstream and compare
   it with `git rev-list --left-right --count HEAD...@{upstream}`.
   - If a clean named branch lacks an upstream, publish it with an ordinary `git push -u` to the
     configured remote.
   - If it is behind only, update with `git pull --ff-only`.
   - If it is ahead only, push it normally.
   - If it has diverged, create unique safety refs, merge the upstream ref without rebasing or
     rewriting history, resolve and validate that integration, commit it, and push normally.
   - If the source is detached and its commit is not contained by a remote ref, create a unique,
     non-overwriting recovery branch at that exact commit and publish the branch before merging it.
5. Recheck clean status and require `0 0` divergence from both upstreams. Never use force or bypass
   a non-fast-forward rejection; fetch and repeat the safe synchronization path instead.

## 3. Reconnaissance and execution plan

After preflight, perform merge-specific reconnaissance:

1. Record `git log -5 --oneline --decorate` for both refs. Inspect their merge base, ahead/behind
   relationship, and `git diff --name-status`, `--stat`, and `--check` for `target...source`.
   Summarize; do not paste large logs. Stop if no merge base exists. If the source has no unique
   commits, verify the target is published and report that it is already integrated instead of
   manufacturing a merge commit.
2. Trace every changed subsystem from definitions, includes, call sites, build manifests, tests,
   help files, and documentation. Do not infer behavior from filenames. Resolve shared/core code
   before dependent feature directories. Remember that files under `src/` use one feature-directory
   level and cross-directory headers require path-qualified includes.
3. If a source file was added, removed, or moved, verify both `Makefile.am` and `CMakeLists.txt`.
   If gameplay or commands changed, verify authoritative help and relevant documentation.
4. Preview the merge without changing the worktree and identify likely conflict paths. Stop only if
   the preview exposes materially ambiguous intent that further tracing cannot resolve.

Create a unique, non-overwriting safety ref such as
`backup/pre-merge-YYYY-MM-DD-HHMMSS` at the exact target HEAD. Never use `-f`.

Present a concise execution update containing the canonical and source paths, exact refs and
commits, feature intent, safety refs, merge command, likely conflict order, expected edit scope,
checks, commit message, and publication target. Then proceed immediately; this update is not an
approval gate.

## 4. Merge execution

Return to the canonical worktree and recheck `pwd`, environment, branch, HEAD, clean status, and
upstream publication. Recheck that the source ref still names the reviewed, published commit. If
either ref changed after reconnaissance, repeat the affected analysis automatically before merging.

If synchronization changed target HEAD, preserve the original safety ref and create a second unique
safety ref at the updated HEAD. Never move or overwrite the original ref.

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
redesign or production/configuration change outside the established merge scope.

Before verification, require an empty unresolved-path list and use `git diff --check` plus a scan of
changed text files for conflict markers. Review both the staged diff and merge status.

At roughly 60 percent of available context, if conflicts or substantial validation remain, do not
commit. Report the safety refs, target and source commits, resolved and unresolved paths, decisions
made, remaining ambiguities, commands run, and test results so another context can continue without
reconstructing the merge.

## 6. Verification, publication, and handoff

Choose targeted tests from the changed subsystem, then run the repository's authoritative checks.
At minimum, unless the user's invocation explicitly requires a narrower check:

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
Diagnose and repair in-scope failures autonomously, then rerun the affected and authoritative gates.
Stop only when a failure requires unavailable external state or a materially broader design choice.

After tests pass:

1. Review `git diff --cached --check`, the full staged diff, and its stat as if reviewing a PR.
   Reject accidental scope, API loss, duplicated registration, stale build lists, disabled tests,
   credential changes, placeholders, and unrelated formatting.
2. Commit the merge with a clear human-authored message describing the feature and notable
   reconciliation. Do not use `--no-verify`. If a hook changes files, review those changes, rerun
   affected checks, stage deliberately, and retry only if the hook rejected the commit. If the
   commit succeeded but left related hook changes, review and validate them before a separate
   follow-up commit. Stop only if their intent is ambiguous. Never silently amend or bypass the hook.
3. Inspect `git diff --stat HEAD^1..HEAD`, `git diff --check HEAD^1..HEAD`, the merge commit
   summary, and final `git status --short --branch`. Require a clean worktree and no unresolved
   markers.
4. Fetch the configured target remote again. Verify the upstream target is an ancestor of the
   validated local target, then publish with an ordinary non-force push. If the push is rejected,
   fetch and repeat the synchronization, conflict-resolution, and validation path; never force.
5. Recheck that the target is clean and has `0 0` divergence from its upstream. Report affected
   subsystems, notable resolutions, tests and exact results, assumptions, safety refs, source and
   merge commits, published target ref, and any focused follow-up review. Flag anything the
   second-pass review would reject.

Do not remove safety refs or delete the source branch/worktree. Do not pause for routine handoff
approval after the validated target has been published.
