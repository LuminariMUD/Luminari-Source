---
name: mergetree
description: >-
  Merge a specified LuminariMUD branch or worktree into the requested target, resolve conflicts,
  and validate the result. Publish only when the user requests publication.
---

# Mergetree

Before any repository or Git operation, read
[references/merge-worktree.md](references/merge-worktree.md) completely and follow it. Treat the
user's instructions and every applicable `AGENTS.md` as authoritative.

Operate according to the requested endpoint. A request to resolve conflicts does not itself
authorize committing or pushing. A request to merge authorizes the scoped local merge and commit;
publish only when requested or already authorized. Do not pause for routine approval when state is
clean, recoverable, and semantically unambiguous. Stop only at the explicit safety boundaries in the
reference.

Ordinary dirty-worktree ambiguity is not a user-facing approval boundary. Preserve uncertain work
losslessly, isolate the merge from it, and continue through publication using the recovery procedure
in the reference. Do not mark the task blocked merely because another process or user has unrelated
changes in the canonical worktree.
