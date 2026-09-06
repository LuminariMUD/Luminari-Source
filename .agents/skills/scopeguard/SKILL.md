---
name: scopeguard
description: >-
  Keep an implementation within its agreed scope when a scope review is requested or work expands
  into speculative design.
---

Complete only the current task within its agreed scope.

Inspect the relevant code, tests, and config needed for the requested change; do not rely on
snippets, guesses, or unverified premises. Resolve uncertainties that affect correctness or scope.
For substantial work, briefly state: **Outcome, Non-goals, Files, and Proof**; use plan-ablation
without duplicating it. Use one implementation path unless parts are truly independent.

Reuse existing code, helpers, patterns, and tests. Fix root causes; preserve unrelated behavior; avoid speculative/future design; add abstractions, adapters, or config only for a second real caller or explicit requirement. Remove replaced code and retain old paths only for required compatibility.

Read-only discovery is allowed. Continue with local edits and tests already authorized by the task,
including requested API or schema changes. Ask before expanding into unrelated dependencies,
frameworks, services, test infrastructure, or duplicate implementations, or before a consequential
scope decision or action not already authorized. Obtain explicit authorization before destructive
data operations, production mutation, discarding user work, rewriting history, or dropping data.

Run the narrowest relevant existing tests and extend existing tests before creating new files. Add tests only for requested or uncovered changed user-observable behavior, with each test protecting a clear acceptance criterion or regression risk. Do not add unrelated coverage or use passing tests to justify extra scope.

If the work grows into future-use layers, workaround stacks, unrelated cleanup, or unstated tests,
remove unnecessary work and keep the requested behavior. Ask only for consequential scope expansion.

Done means the requested behavior and acceptance criteria pass; exact commands/results are reported;
every touched file is necessary; the diff contains nothing unrelated; no task-created debug or
scratch artifacts remain in the deliverable; required recovery evidence and unrelated work are
preserved and reported; and assumptions, limitations, and unverified runtime behavior are stated
plainly.
