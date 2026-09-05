---
name: scopeguard
description: >-
  Complete an implementation with the smallest sufficient changes.
---

Complete only the current task with the smallest sufficient change.

Before editing, inspect the relevant code, tests, and config directly; do not rely on snippets, guesses, or unverified premises. Resolve ambiguity first, then state: **Outcome, Non-goals, Files, and Proof**. Use one implementation path unless parts are truly independent.

Reuse existing code, helpers, patterns, and tests. Fix root causes; preserve unrelated behavior; avoid speculative/future design; add abstractions, adapters, or config only for a second real caller or explicit requirement. Remove replaced code and retain old paths only for required compatibility.

Read-only discovery is allowed. Get approval before expanding scope or touching unrelated files; adding dependencies, frameworks, services, or test infrastructure; changing public APIs, schemas, storage, or wire formats; deleting/overwriting data, discarding uncommitted work, rewriting history, or dropping data; or keeping duplicate implementations.

Run the narrowest relevant existing tests and extend existing tests before creating new files. Add tests only for requested or uncovered changed user-observable behavior, with each test protecting a clear acceptance criterion or regression risk. Do not add unrelated coverage or use passing tests to justify extra scope.

If the work grows into future-use layers, workaround stacks, unrelated cleanup, or unstated tests, stop, shrink the plan, and confirm scope.

Done means the requested behavior and acceptance criteria pass; exact commands/results are reported; every touched file is necessary; the diff contains nothing unrelated; no debug, backup, dead, or scratch files remain; and assumptions, limitations, and unverified runtime behavior are stated plainly.
