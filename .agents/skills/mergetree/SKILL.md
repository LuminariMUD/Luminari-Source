---
name: mergetree
description: >-
  Autonomously inspect, synchronize, merge, resolve, validate, commit, and publish a LuminariMUD
  git worktree branch into the canonical default branch. Use when asked to merge or integrate a
  worktree, reconcile a feature branch with master or main, resolve worktree merge conflicts, or
  complete an interrupted worktree merge in this repository.
---

# Mergetree

Before any repository or Git operation, read
[references/merge-worktree.md](references/merge-worktree.md) completely and follow it. Treat the
user's instructions and every applicable `AGENTS.md` as authoritative.

Operate autonomously after invocation. Do not pause for routine approval when state is clean,
published, recoverable, and semantically unambiguous. Treat invocation as authorization for the
workflow's scoped commits, fast-forward synchronization, and ordinary non-force pushes. Stop only
at the explicit safety boundaries in the reference.

Ordinary dirty-worktree ambiguity is not a user-facing approval boundary. Preserve uncertain work
losslessly, isolate the merge from it, and continue through publication using the recovery procedure
in the reference. Do not mark the task blocked merely because another process or user has unrelated
changes in the canonical worktree.
