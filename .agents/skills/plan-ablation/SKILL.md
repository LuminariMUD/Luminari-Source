---
name: plan-ablation
description: Simplify a coding plan by checking what can be removed while still meeting the task's requirements. Use after planning and before implementation.
---

# Plan ablation

Review the proposed plan before editing code. Ablation means mentally removing a planned part
and checking what would fail; it does not require implementing multiple versions.

- For each meaningful change, ask: if we omit this, which requirement or concrete correctness
  risk goes unmet? Remove it if there is no concrete answer.
- Look for a simpler solution using existing code and patterns. Question new abstractions,
  dependencies, configuration, fallback paths, and work justified only by hypothetical future use.
- Keep changes and checks needed for the requested behavior, safety, compatibility, and
  repository requirements. Fewer lines alone do not make a solution better.

Briefly state what to remove or simplify and why, then update the plan before proceeding.
If nothing can be removed, say why the plan is already minimal. A sentence is enough for a
small task; do not create a separate report or add an approval step.
