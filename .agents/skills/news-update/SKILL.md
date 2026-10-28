---
name: news-update
description: >-
  Update LuminariMUD player news from landed repository history and publish it to the
  authorized environment. Use for missing announcements, release news, or a news catch-up,
  with comprehensive, readable player-facing coverage and Luminari color formatting.
---

# Luminari News Update

Update `lib/text/news` unless the user requests a draft or review only. Tell players what
changed, who it affects, how to use it, and the relevant costs, limits, and exceptions.
Account for every distinct player-facing change without turning the news into a commit log.
Creating or editing this skill does not publish news or authorize production access.

Paths below are relative to the repository root. Read `AGENTS.md`, inspect Git status,
and preserve unrelated edits. Before local writes, read only `APP_ENV` from `lib/.env`
without printing credentials. A normal update publishes to the verified current environment;
a development checkout does not imply permission to publish remotely. Reuse explicit
production authorization already given, including a deployment request.

## Establish the destination and coverage

Trace the current news path through `NEWS_FILE` in `src/core/db.h`, loading in
`src/core/db.c`, and `SCMD_NEWS` in `src/act/act.informative.c`. Luminari loads news
from a text file into memory; it does not use Duris's database news row or import script.
Resolve the running process's runtime directory before changing a live file.

Read the destination's existing news, including the latest sections and older archive.
The development file can differ from production. Preserve a private recovery copy before
editing. `lib/text/news` is currently gitignored, so Git status and a path-limited Git diff
alone do not show its changes. Check `git ls-files` and `git check-ignore`; use a direct
before/after diff when untracked. Do not force-add runtime news or change ignore rules.

Discover the source repository and release branch from Git remotes and repository metadata.
Default to landed default-branch history, or the exact release SHA supplied by a deployment.
Pin the full source tip SHA. Use the connected repository tools for PR context when available,
and Git or a paginated API for a complete commit inventory.

Establish the last substantive coverage update by comparing existing announcements with
source changes. If the file has history, inspect `git log --follow -- lib/text/news` and
its relevant diffs. A formatting edit, file timestamp, or latest date is not proof of
complete coverage. For an ignored file, use the existing content and verified release
records; do not invent a baseline. Recheck overlap for omissions and same-day changes.
If no reliable baseline exists, widen the review and report its actual boundary.

Enumerate all commits in the verified `BASE_SHA..TIP_SHA` ancestry range and reconcile
the inventory count with `git rev-list --count BASE_SHA..TIP_SHA`. Include direct commits,
squash merges, branch changes, and merge resolutions. A date search, first-parent log,
PR-title list, or capped recent-results query is not a complete inventory. Inspect
`git show SHA:path` where the local checkout differs from the pinned release.

Keep working coverage notes keyed by SHA or PR; grouped PRs must retain their member SHAs.
Assign each distinct change a disposition:

- Included: identify its new or corrected news bullet.
- Already covered: identify the existing section and bullet.
- Superseded or reverted: identify the later change and surviving behavior.
- No player effect: give a specific reason, such as tests alone or a behavior-preserving move.
- Unresolved: investigate the missing evidence before claiming complete coverage.

Read patches and relevant source, help, settings, and world data at the pinned tip.
PR descriptions explain intent; landed behavior determines the claim. Follow fixes and
reversions together. Never announce open PRs, proposals, disabled features, test fixtures,
or unpublished local code as available. If an authorized deployment supplies separately
published help additions, verify their diff and in-game lookup and account for them as
content additions, without calling them merged history.

Sweep the changed paths for combat, classes/races, spells/skills/feats, pets, gear/crafting,
loot/storage, quests/rewards, travel/vessels, clans, accounts, clients, displays, and help.
Include restrictions and repaired player problems as well as new abilities. Do not turn
internal cleanup into unsupported promises of speed, stability, or lost-item prevention.

## Write for players

Use a welcoming, knowledgeable voice and speak directly to the player. Lead each bullet
with the practical change, then explain affected characters, conditions, and useful commands.
Keep humor brief and balance reductions plain. Group related fixes without losing distinct
consequences. There is no arbitrary bullet limit; tighten wording before dropping coverage.

Verify eligibility, levels, targets, costs, durations, uses, and restrictions. Preserve
command spelling and argument order. A pulse or tick is not automatically a second or round.
Explain unfamiliar game terms. Replace implementation jargon with observable behavior;
leave paths, function names, SHAs, issue numbers, and testing narratives in the operator report.

Keep the newest date block above earlier sections, including multiple updates on the same
day. Put the newest headline and main addition on the first default-height page. Use verified
landing dates and the user's timezone where relevant. Split a large catch-up into useful
dated blocks. Preserve history, correcting factual errors only with evidence. On repeat runs,
reconcile existing bullets and avoid duplicate announcements or date-only churn.

Write final release prose. Keep temporary deployment/reboot status out of the news; report
activation separately. Do not infer that merged code is already running in production.

## Use Luminari text and colors

Follow the destination file's layout. Older local news may be plain text; use restrained
Luminari colors for new sections without recoloring the archive. Trace `COLOUR_CHAR` and
`GetColorCode()` in `src/net/protocol.h` and `src/net/protocol.c` if the format has changed.
The current renderer accepts `^W` for white rules, `^Y` for titles and bullet markers,
`^C` for topics and commands, `^R` for dates or warnings, and `^n` to reset. Use `^^` for
a literal caret. Do not copy Duris `&+` tokens or write literal backslash-t escapes.

Use ASCII text, UTF-8 encoding, LF endings, and a final newline. No Markdown headings,
HTML, terminal escape bytes, emoji, or enclosing code fences in the news file. Reset
colored spans and headings. Wrap to roughly 70-76 visible columns, at most 78 where
practical, counting color tokens as zero width; keep command syntax together when it fits.
Use two-space continuation indentation and blank lines between topics.

For example, adapting the date and content to verified evidence:

```text
^Y                         LUMINARIMUD PLAYER NEWS^n
^WUpdated: Month DD, YYYY^n

^R[Month DD, YYYY]^n ^YShort, useful release headline^n
^W----------------------------------------------------------------------^n

^CA topic players recognize^n
^Y-^n Practical change, with ^C'command'^n and its conditions when useful.
  A short continuation explaining an important limit or consequence.
```

Read a color-stripped preview, including the first page. Keep the full file comfortably
below the loader's 512 KiB cap, allowing for its read buffer and LF-to-CRLF expansion;
trace `file_to_string_alloc()` before approaching that limit. Do not discard the archive
to make room without an agreed archival destination.

## Publish and prove the result

Reconcile history to news for omissions and news to source for unsupported or stale claims.
Recheck the remote tip for a default-branch catch-up and review any additional range.
For a deployment, retain its pinned release cutoff even if the branch advances. If remote
access fails, state the exact verified boundary rather than claiming full coverage.

Review the complete before/after diff, widths, dates, color resets, file size, and encoding.
For a tracked file run `git diff --check -- lib/text/news`; for an ignored file also compare
against the recovery copy and check whitespace directly. If production authorization is
missing, finish the draft, preview, and recovery preparation before requesting permission
for the specific destination. Honor draft-only requests without writing the live file.

Publish only the reviewed news file, retaining its ownership and permissions. Recheck the
destination against the captured original before replacing it so concurrent edits survive.
Use a same-directory temporary file and atomic rename, then verify destination bytes against
the reviewed candidate. News publication does not require a database import or help sync.

Log in using the authorized staff account without exposing credentials. The registered
reload command is **`reloadimm news`** (`LVL_IMPL`), not `reboot news` or Duris's `page`.
Trace `cmd_info[]` and `do_reboot()` before use. The loader can refuse while a player is
paging the existing news; allow the pager to finish and retry the reload rather than
restarting the server. A generic success response is not sufficient: check for read errors
and run the actual `news` command. If an authorized deployment will immediately activate
the release, perform this readback after startup loads the file.

Confirm the newest headline and main addition on the first page, then page through the
whole new section. Restore any display preferences changed for verification, quit, and
close the session. Report file publication and cached/player-visible verification separately
if the staff login or reload could not be completed. A matching file alone is not full success.

Report the destination, covered period, baseline/tip SHAs, useful source links, preview and
readback results, and remaining gaps. Keep working coverage notes out of player news and
do not add a tracking framework. A standalone news update does not restart the game,
commit/push, process feedback, or announce on external channels.
