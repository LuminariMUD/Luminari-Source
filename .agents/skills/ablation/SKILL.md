---
name: ablation
description: >-
  Simplify implementation plans and changes while preserving the full requested outcome. Use
  after planning and before implementation, for scope reviews, or when work accumulates
  speculative layers, workarounds, or unrelated changes.
---

# Ablation

Deliver the complete requested outcome through the simplest correct implementation. Reduce
unnecessary machinery and work without reducing the user's ambition or acceptance criteria.
Fewer lines, fewer files, and green tests alone do not establish a better solution.

Ablation is a counterfactual check: mentally remove or replace a meaningful part, then trace
what would fail. It does not require building multiple implementations or running an experiment
for every decision. Apply it to a plan before editing and to an implementation when new evidence
changes the plan or scope starts to grow.

## Establish what must survive

- Read the actual request, relevant prior decisions, and acceptance criteria. Separate required
  outcomes and constraints from suggested implementation choices and assumptions. Preserve the
  whole objective; difficulty, elapsed effort, or existing test coverage cannot redefine it.
- Inspect the relevant code, callers, tests, configuration, and repository instructions. Trace
  the affected path far enough to establish behavior and dependencies; names and snippets are
  insufficient. Investigate uncertainties that could change correctness, scope, or the approach.
- Inspect existing changes before editing. Distinguish task-owned work from unrelated user work
  so that later simplification removes only work this task owns.
- For substantial work, briefly state the outcome, scope boundaries, likely files or execution
  path, and proof needed. Reuse the current plan or conversation. For small changes, a sentence
  is enough; do not create a separate report, checklist, or approval stage.

## Challenge the proposed work

For each meaningful change, abstraction, dependency, configuration option, fallback, or check,
ask: **If this is omitted, which requirement or concrete failure becomes unaddressed, and what
evidence supports that answer?** A requirement, traced consumer, reproduced defect, or applicable
repository rule is a reason to retain work. Hypothetical future use is not.

Choose a disposition based on that evidence:

| Decision | When it fits |
| --- | --- |
| Keep | It supplies required behavior, compatibility, safety, recovery, or verification. |
| Simplify | The obligation is real, but an existing path or smaller mechanism can satisfy it. |
| Drop | It has no requirement or demonstrated current risk to address. |
| Investigate | Missing evidence could change the decision; inspect or reproduce the specific uncertainty. |

Evaluate related parts together. A framework can appear necessary only because its adapter,
configuration, and tests depend on it. Check whether the entire cluster can be removed or
replaced. Conversely, retain supporting changes needed for a required migration, integration,
or recovery path even when they do not expose a separate user feature.

Prefer the existing implementation path and fix the root cause. Reuse helpers, patterns, and
tests; keep one implementation unless independent responsibilities or required compatibility
justify more. Add an abstraction, adapter, or configuration surface only for an explicit
requirement, a real additional consumer, or a demonstrated current correctness boundary.
Do not introduce such layers solely to make a small change look general.

Compare total complexity: branches, state, dependencies, public interfaces, operational burden,
and future maintenance. A shorter patch that hides behavior, piles on workarounds, or leaves
two competing paths may be worse. A necessary multi-file repair belongs in scope even when a
local symptom patch would be smaller. Preserve unrelated behavior.

Before proceeding, check both directions: every retained change has a concrete purpose, and
every requirement still has an implementation path and appropriate proof. Briefly state what
was removed or simplified and why, then update the plan. If nothing can be removed, say why
the plan is already minimal. Do not manufacture a deletion quota.

## Keep the implementation aligned

Carry out the revised plan when implementation is authorized. A request to review or simplify
a plan authorizes that review; it does not by itself authorize implementing the proposed code.
Read-only discovery can resolve scope questions without creating an implementation commitment.

When new evidence exposes a missing requirement or a defect blocking the requested workflow,
include the necessary repair and update the plan. Unrelated cleanup, future-use layers, new
services, or general test infrastructure need their own justification. If they are optional,
omit them rather than interrupting delivery to seek approval for extra work.

If work starts accumulating fallback paths, workaround layers, duplicate implementations, or
unstated tests, reassess the cause and simplify the task-owned changes. Remove replaced code,
stale callers, and configuration made obsolete by this task; retain old paths only for an
identified compatibility obligation. Inspect the actual diff before pruning, including when
user changes share a file. Do not discard unrelated work or required recovery evidence.

Use authorization already present in the conversation for necessary local edits and checks,
including requested API or schema changes. Do not add a confirmation step for routine choices.
Ask only when a consequential unresolved choice would change the promised outcome or the next
action exceeds existing authorization. Destructive data operations, production mutation,
discarding user work, and history rewriting require explicit authorization for that action;
this skill supplies none. Continue independent authorized work while a required answer is pending.

## Prove the whole outcome and stop

- Run the narrowest relevant existing checks during implementation and all required completion
  checks. Reuse or extend existing tests before adding files or infrastructure. Add tests for
  changed observable behavior, acceptance criteria, or a concrete regression risk; avoid tests
  that merely repeat the implementation or add unrelated coverage.
- Match proof to the claim. A helper test does not prove a feature is reachable through its
  command, API, or workflow. Check the real integration path when that is part of acceptance.
  Update required documentation, help, manifests, or migrations alongside the behavior they serve.
- Reuse valid results. Repeat or broaden verification when relevant changes, failures, unresolved
  risks, or required gates justify it. Once the required evidence holds, finish delivery instead
  of rerunning green suites or inventing a new hardening phase.
- Audit the current result against every original requirement and accepted scope change. Missing,
  skipped, or indirect evidence leaves the corresponding claim unverified. Passing a smaller
  suite cannot justify dropping a requirement or declaring a partial implementation complete.
- Inspect the final diff: every task-owned file and change must be necessary; replaced machinery
  and task-created scratch/debug artifacts must be absent from the deliverable. Preserve unrelated
  work and required recovery evidence. Do not perform repository-wide cleanup to achieve tidiness.

Report the concrete outcome and useful simplifications, the exact verification commands and
results, and any material assumptions, limitations, or unverified runtime behavior. Mention
preserved unrelated work or recovery evidence when it affects review or follow-up. Completion
means the requested outcome is proven and delivered, with no required work left open.
