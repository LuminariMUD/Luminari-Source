# Known Issues

Intentional exceptions to automated checks. Every entry records why it is
exempt and when it was added. Remove entries that no longer apply.

## Ignored Paths

| Pattern | Reason | Added |
|---------|--------|-------|

## Ignored Rules

| Tool | Rule | Scope | Reason | Added |
|------|------|-------|--------|-------|

## Known Failing Tests

| Test | Reason | Added |
|------|--------|-------|

## Skipped Workflows

| Workflow | Reason | Added |
|----------|--------|-------|

## Skipped Infra

| Item | Reason | Added |
|------|--------|-------|
| Production Health probe | This checkout is development and repository policy forbids production code changes. The endpoint and rendered systemd probe passed against an isolated local MariaDB runtime; remove this entry after the released unit is installed, restarted, and probed in production. | 2026-08-07 |
