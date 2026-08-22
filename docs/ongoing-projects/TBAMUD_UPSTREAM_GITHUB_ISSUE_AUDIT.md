# tbaMUD Upstream GitHub Issue Audit

Audit date: 2026-08-23

Upstream source: <https://github.com/tbamud/tbamud/issues>

Local source revision: `b7a2c6da4fc71d3c9199450bb79c074ac48159bb`

## Purpose and scope

This document checks every GitHub issue in `tbamud/tbamud`, open and closed,
against the current LuminariMUD source tree. Pull requests are not counted as
issues. The inventory was produced with:

```text
gh issue list -R tbamud/tbamud --state all --limit 1000
```

The snapshot contains 45 issues: 12 open and 33 closed. Each issue is assigned
one of these local dispositions:

- Confirmed defect: the reported defect, or a directly equivalent failure,
  exists in current LuminariMUD code.
- Present by design: the behavior or concern exists, but is intentional,
  cosmetic, architectural, or legal rather than a demonstrated source defect.
- Not present: the relevant code is fixed, replaced, or behaves differently.
- Not applicable: the report concerns upstream infrastructure, a reporter's
  private modifications, support, content not shipped by LuminariMUD, or an
  upstream-only project task.

## Executive result

Four current defects were confirmed:

| Priority | Issue | Finding |
| --- | --- | --- |
| P0 | [#181](https://github.com/tbamud/tbamud/issues/181) | Zone names reach `system()` after an incomplete filename sanitizer, allowing shell metacharacters. |
| P0 | [#157](https://github.com/tbamud/tbamud/issues/157) | PvP policy is not enforced at the central combat transition, and `grapple` is a confirmed player-accessible bypass. |
| P1 | [#92](https://github.com/tbamud/tbamud/issues/92) | VNUM storage is 32-bit, but room/mobile/object stat output still assumes narrow VNUM fields and often uses signed `%d` for unsigned index types. |
| P2 | [#57](https://github.com/tbamud/tbamud/issues/57) | The bundled `snprintf` fallback still formats fractional values with leading zeroes incorrectly. It is dormant on the current development host. |

Six additional reports still describe this repository, but are not classified
as correctness defects:

- [#47](https://github.com/tbamud/tbamud/issues/47): rooms still have both
  room-file and zone-reset trigger attachment mechanisms.
- [#80](https://github.com/tbamud/tbamud/issues/80): `skillset` deliberately
  permits an administrator to override a level requirement after warning.
- [#86](https://github.com/tbamud/tbamud/issues/86): separately dropped coin
  piles remain separate objects.
- [#90](https://github.com/tbamud/tbamud/issues/90): `IDXTYPE` is 32-bit on the
  configured target, but is still spelled `unsigned int` rather than a
  fixed-width `uint32_t` type.
- [#95](https://github.com/tbamud/tbamud/issues/95): inherited licensing remains
  relevant and is already called out in the repository's legal documentation.
- [#105](https://github.com/tbamud/tbamud/issues/105): new room objects are
  inserted at the head of the room list, so recent drops appear above fixtures.
  This behavior also avoids upstream issue #147.

The other 35 issues are fixed, absent, or not applicable to this codebase.

## Confirmed defects

### P0: Issue #181 - command injection in zone export

Status: Confirmed.

The local [`fix_filename()`](../../src/olc/genolc.c#L824) replaces spaces and
parentheses and drops quotes, but passes shell characters such as semicolon,
pipe, ampersand, backtick, dollar, redirection characters, backslash, and line
breaks unchanged. [`do_export_zone()`](../../src/olc/genolc.c#L866) then embeds
that value in `rm`, `tar`, and `gzip` command strings passed to `system()`.

The export command requires implementor level, but a builder can control the
zone name. This matches the upstream builder-plus-implementor attack described
in #181. The MUD process executes the resulting command as its operating-system
user.

Recommended direction:

1. Stop invoking a shell for archive creation and deletion. Use explicit
   argument arrays with `execv()`/`posix_spawn()`, or a suitable archive API.
2. Independently restrict export basenames to a small allowlist such as ASCII
   letters, digits, underscore, hyphen, and dot.
3. Add a regression test containing every shell metacharacter and a line break.

### P0: Issue #157 - PvP policy bypasses

Status: Confirmed equivalent failure.

LuminariMUD has a stronger mutual-consent policy in
[`pvp_ok()`](../../src/utils.c#L6946), and common attack and spell paths call it.
The policy is not enforced in the central
[`set_fighting()`](../../src/combat/fight.c#L1679) transition, however. When
`CONFIG_PK_ALLOWED` is false, `set_fighting()` only calls `check_killer()`;
that function's killer-flag mutation is commented out and it does not reject
combat.

[`do_grapple()`](../../src/combat/grapple.c#L208) is a concrete bypass. It
selects a visible player, performs and applies the maneuver, and calls
`set_fighting()` in both directions without calling `pvp_ok()`. Consequently a
player can initiate hostile state and grapple effects while global PK is off,
or without mutual PvP consent when PK is on.

This confirms the central concern in #157 even though LuminariMUD's desired
policy differs from upstream's three-state design.

Recommended direction:

1. Audit every hostile entry point, including maneuvers, area callbacks,
   damage shields, pets, spells, and vessel combat.
2. Add a defensive policy check at the lowest safe combat-entry layer, while
   retaining enough context to distinguish an initiating attack from the
   victim's automatic retaliation.
3. Add production-linked tests for PK off, one-sided consent, mutual consent,
   arena exceptions, player pets, and `grapple` specifically.

### P1: Issue #92 - large-VNUM stat output is incomplete

Status: Confirmed.

Issue #90's storage range is available on the configured target:
[`IDXTYPE`](../../src/structs.h#L59) is `unsigned int`, which is 32-bit here.
It is not the fixed-width `uint32_t` requested by #90, so that issue remains a
portability concern rather than a current-host failure. Zone creation accepts
values up to `IDXTYPE_MAX / 100`, and
[`atoidx()`](../../src/utils.c#L4512) rejects negative and out-of-range input.

The display conversion is incomplete. Examples include:

- [`do_stat_room()`](../../src/act.wizard.c#L878), which prints zone, VNUM, and
  RNUM with widths of three or five characters.
- [`do_stat_character()`](../../src/act.wizard.c#L1054), which prints room and
  mobile VNUMs with five-character fields.
- [`oasis_list.c`](../../src/olc/oasis_list.c#L1417), which contains several
  fixed five- or seven-character VNUM layouts.

These widths are minimum widths, so large values are not truncated, but they do
break the intended column alignment and can push output beyond the issue's
120-column target. Many of these calls also pass unsigned `IDXTYPE` values to
signed `%d` conversions.

Recommended direction: define one index-format convention, use unsigned or
fixed-width format macros consistently, size columns for the supported range,
and add rendered-output tests using zone 600000 and VNUMs 60000000-60000099.

### P2: Issue #57 - fallback floating-point formatting

Status: Confirmed but dormant in the current environment.

The fallback [`fmtfp()`](../../src/bsd-snprintf.c#L564) converts the fractional
integer into reverse-order digits, then emits those digits before its zero
padding. For a value such as `2.01`, the fractional integer is `1`; the code
therefore emits `1` followed by zeroes instead of emitting the missing leading
zeroes before `1`.

The configured development build defines both `HAVE_SNPRINTF` and
`HAVE_VSNPRINTF`, so libc is used and this path is not active here. The defect
still exists in shipped source for platforms that use the fallback.

Recommended direction: add `2.01`, `2.001`, and negative equivalents to the
built-in fallback tests and emit fractional zero padding before the reversed
digits.

## Present behavior and non-defect decisions

### Issue #47 - two room-trigger attachment mechanisms

Status: Present architectural behavior.

[`save_rooms()`](../../src/olc/genwld.c#L395) writes room prototype scripts into
`.wld` files, while [`reset_zone()`](../../src/db.c#L5621) also processes `T`
commands from `.zon` files and can attach a trigger to a room during reset.
Thus the two mechanisms described by #47 still exist and have different
reattachment behavior.

This is real builder-facing complexity, but collapsing the mechanisms would be
a compatibility change. A safe first step is to document the distinction and
detect duplicate or conflicting attachments during OLC save and world boot.

### Issue #80 - `skillset` continues after a level warning

Status: Intentional administrator override.

[`do_skillset()`](../../src/modify.c#L494) warns when a skill's minimum level is
above the target's level, then assigns the requested value. It does return for
skills unavailable to mortals. This matches the upstream maintainer's stated
testing/admin intent. The missing return is not a defect, though clearer output
would make the override explicit.

### Issue #86 - separate coin piles

Status: Present cosmetic behavior.

[`perform_drop_gold()`](../../src/obj/act.item.c#L2697) creates a new money
object for each drop, and [`obj_to_room()`](../../src/handler.c#L2738) does not
coalesce money objects. Upstream closed this as `wontfix` because custom money
objects and object properties make automatic merging behavior-sensitive.

### Issue #95 - LGPL relicensing

Status: Applicable legal/project concern, not a source defect.

The root [`LICENSE`](../../LICENSE) and
[`docs/legal/README_legal.md`](../legal/README_legal.md) already distinguish
LuminariMUD's public-domain custom code from inherited tbaMUD, CircleMUD,
DikuMUD, and other licensed content. Upstream relicensing cannot be inferred
from the newer licenses of its ancestors; contributor permission and code
provenance remain upstream concerns. This audit makes no legal conclusion.

### Issue #105 - room object display order

Status: Present ordering policy.

[`obj_to_room()`](../../src/handler.c#L2738) inserts each object at the head of
`world[room].contents`. The newest dropped object therefore appears before
older fixtures such as boards and fountains. That is the behavior reported in
#105.

Current upstream tbaMUD later changed to append objects, which led to open issue
[#147](https://github.com/tbamud/tbamud/issues/147): autoloot can select an older
corpse. LuminariMUD's head insertion avoids #147. Changing order should be an
explicit player-compatibility decision, not treated as an isolated bug fix.

## Complete issue disposition

| Issue | Upstream | Local disposition | Evidence and conclusion |
| --- | --- | --- | --- |
| [#2 Pagelength](https://github.com/tbamud/tbamud/issues/2) | Closed | Not present | The pager accepts 255 and resets only values above 255; the maximum-page regression test exercises 255. |
| [#3 Olist Sword](https://github.com/tbamud/tbamud/issues/3) | Closed | Not present | Object-name and affect lists stop with a safety margin before another append; generic OLC list appenders also cap their buffers. |
| [#47 Room trigger attachment](https://github.com/tbamud/tbamud/issues/47) | Closed | Present by design | Both room-file prototype scripts and zone-reset `T` commands remain; see the detailed section above. |
| [#56 Mobile save comparison](https://github.com/tbamud/tbamud/issues/56) | Closed | Not present | The reported aggregate-record length comparison no longer exists. `write_mobile_record()` writes bounded description copies and fields directly to `FILE`. |
| [#57 Float 2.01](https://github.com/tbamud/tbamud/issues/57) | Closed | Confirmed defect | The bundled fallback still places fractional zero padding after significant digits. |
| [#59 Website download link](https://github.com/tbamud/tbamud/issues/59) | Closed | Not applicable | Upstream website/release policy only. |
| [#77 Missing syslogs](https://github.com/tbamud/tbamud/issues/77) | Closed | Not applicable | The reporter had skipped configure/build; no product defect was identified. |
| [#78 Affect flag count](https://github.com/tbamud/tbamud/issues/78) | Closed | Not present | Affect indexes consistently use `1 <= index < NUM_AFF_FLAGS`; constant tables include the sentinel entry and compile-time size checks. |
| [#79 `how_good` typo](https://github.com/tbamud/tbamud/issues/79) | Closed | Not present | The reported helper and malformed string are no longer in this tree. |
| [#80 `skillset` return](https://github.com/tbamud/tbamud/issues/80) | Open | Present by design | The continuation is an administrator override after a warning, not a failed guard. |
| [#81 Syntax-check crash](https://github.com/tbamud/tbamud/issues/81) | Closed | Not present | `queue_free(NULL)` returns safely, and the syntax-check boot path completed during this audit. |
| [#83 `wpurge` dropped gold crash](https://github.com/tbamud/tbamud/issues/83) | Closed | Not present | Drop paths save the object script ID and call `has_obj_by_uid_in_lookup_table()` after triggers before dereferencing or extracting the object. |
| [#85 GCC 9.2 warnings](https://github.com/tbamud/tbamud/issues/85) | Closed | Not present | `name_from_drinkcon()` uses bounded allocation/memory copies, and `say_spell()` uses larger bounded buffers. The reported warning sites are gone. |
| [#86 Multiple gold stacks](https://github.com/tbamud/tbamud/issues/86) | Closed | Present by design | Each drop creates a distinct money object; upstream classified this as cosmetic and `wontfix`. |
| [#89 Testing frameworks](https://github.com/tbamud/tbamud/issues/89) | Open | Not applicable/satisfied | This is an upstream planning task. LuminariMUD already has production-linked CuTest integration tests and focused harnesses. |
| [#90 Change IDXTYPE to uint32](https://github.com/tbamud/tbamud/issues/90) | Open | Present portability concern | `IDXTYPE` is `unsigned int`, which is 32-bit on the configured target but is not a fixed-width `uint32_t` type. |
| [#91 OLC with uint32](https://github.com/tbamud/tbamud/issues/91) | Open | Not present at stated target | `atoidx()` and `create_new_zone()` support zone 600000 and rooms 60000000-60000099. Display formatting remains incomplete under #92. |
| [#92 Stat alignment for uint32](https://github.com/tbamud/tbamud/issues/92) | Open | Confirmed defect | Stat and OLC output still use narrow fields and signed conversions for unsigned indexes. |
| [#93 Large test world](https://github.com/tbamud/tbamud/issues/93) | Open | Not applicable | Upstream performance-test data request, not an inherited runtime defect. |
| [#95 Relicensing to LGPL](https://github.com/tbamud/tbamud/issues/95) | Open | Present legal concern | Inherited licensing is explicitly tracked locally; no code defect or automatic relicensing follows. |
| [#96 Typo/idea/bug crash](https://github.com/tbamud/tbamud/issues/96) | Closed | Not applicable | The report arose from the reporter's Korean command-order and character-set modifications; stock upstream did not reproduce it. |
| [#98 Utility overruns](https://github.com/tbamud/tbamud/issues/98) | Closed | Not present at reported sites | `shopconv` uses bounded formatting and the obsolete `webster` utility is absent. |
| [#104 Forged item persistence](https://github.com/tbamud/tbamud/issues/104) | Closed | Not present | The report did not identify an upstream defect. LuminariMUD object saving explicitly persists changed values and affect records relative to the prototype. |
| [#105 Room object ordering](https://github.com/tbamud/tbamud/issues/105) | Closed | Present by design | Head insertion makes recent drops appear first and avoids #147's stale-corpse autoloot behavior. |
| [#106 Zone list documentation](https://github.com/tbamud/tbamud/issues/106) | Closed | Not applicable | The list is upstream world content. LuminariMUD ships a different world and exposes area/zone information in game. |
| [#107 Startup SYSERR](https://github.com/tbamud/tbamud/issues/107) | Closed | Not applicable | Reporter confirmed the error came from copying and modifying `do_exits`; it was not in stock source. |
| [#108 Startup SCRIPT ERROR](https://github.com/tbamud/tbamud/issues/108) | Closed | Not applicable | Same reporter modification as #107; not a stock defect. |
| [#109 `look_at_room` braces](https://github.com/tbamud/tbamud/issues/109) | Closed | Not present | `look_at_room()` has been substantially rewritten with explicit blocks; the reported misleading indentation is absent. |
| [#116 Error message detail](https://github.com/tbamud/tbamud/issues/116) | Closed | Not present | `wdoor`, `mdoor`, and `odoor` errors include the invalid argument and valid-choice list or bad target value. |
| [#124 Website down](https://github.com/tbamud/tbamud/issues/124) | Closed | Not applicable | Upstream website operations only. |
| [#129 Maps for tbaMUD](https://github.com/tbamud/tbamud/issues/129) | Closed | Not applicable | Upstream world-content resource, not LuminariMUD code. |
| [#135 Nested DG loop freeze](https://github.com/tbamud/tbamud/issues/135) | Closed | Not present | Each command-list element has its own loop counter; a false nested `while` resets only that line's counter, and loop execution yields after 30 iterations. |
| [#141 `where` truncation](https://github.com/tbamud/tbamud/issues/141) | Closed | Not present | `perform_immort_where()` appends to a dynamically growing buffer and `deliver_where_output()` pages the completed result. |
| [#144 `aedit.c` warning](https://github.com/tbamud/tbamud/issues/144) | Closed | Not present | The assignment correctly remains outside a braced conditional that only frees the previous string. Putting it inside would fail for a null old value. |
| [#147 Autoloot and `obj_to_room`](https://github.com/tbamud/tbamud/issues/147) | Open | Not present | LuminariMUD inserts new corpses at the head, so normal lookup sees the newest corpse first. This is the inverse tradeoff of #105. |
| [#148 `name_from_drinkcon` prototype pollution](https://github.com/tbamud/tbamud/issues/148) | Closed | Not present | The function frees `obj->name` only for unprototyped objects or when the pointer differs from the prototype name. |
| [#155 Website email failure](https://github.com/tbamud/tbamud/issues/155) | Closed | Not applicable | Upstream website mail configuration only. |
| [#157 PK configuration bypass](https://github.com/tbamud/tbamud/issues/157) | Closed | Confirmed defect | Central combat entry does not reject PvP, and `grapple` is a verified unchecked caller. |
| [#159 Questmaster retains return items](https://github.com/tbamud/tbamud/issues/159) | Closed | Not present | `AQ_OBJ_RETURN` completes the quest and calls `extract_obj(object)`. |
| [#179 Complex alias overflow](https://github.com/tbamud/tbamud/issues/179) | Closed | Not present | Every token, `$*`, literal, and escaped-dollar write checks remaining `MAX_RAW_INPUT_LENGTH`; overflow frees the temporary queue and returns failure. |
| [#181 Zone export command injection](https://github.com/tbamud/tbamud/issues/181) | Open | Confirmed defect | Incomplete sanitization feeds attacker-controlled zone-name bytes to three `system()` calls. |
| [#183 `var_subst` stack overflow](https://github.com/tbamud/tbamud/issues/183) | Open | Not present | Input is copied to the 512-byte temporary with `strlcpy`, and output is bounded by the `left` counter. Oversized lines truncate rather than overwrite the stack. |
| [#185 `sizeof(pointer)` null write](https://github.com/tbamud/tbamud/issues/185) | Closed | Not present | The function terminates at the current output cursor with `*buf = '\0'`; it does not use `sizeof(buf)`. |
| [#186 Copyover `fscanf` widths](https://github.com/tbamud/tbamud/issues/186) | Open | Not present | The format uses `%511s`, `%1023s`, and `%1023s` for the corresponding arrays and validates that five fields were read. |
| [#188 Player-file password copy](https://github.com/tbamud/tbamud/issues/188) | Open | Not present | Password loading uses `strlcpy(GET_PASSWD(ch), line, sizeof(ch->player.passwd))`. |

## Verification

- The upstream issue inventory was refreshed after the source review: 45 issues,
  comprising 12 open and 33 closed issues. The table contains each issue number
  exactly once and its recorded state matches the refreshed GitHub data.
- `./bin/luminari -c -d lib` completed successfully, loading 762 zones, 91,736
  rooms, 27,069 mobiles, and 22,637 objects before clean shutdown.
- `make test` passed all 793 production-linked CuTest tests. The required
  follow-up `make install` also completed successfully.
- The report passes the repository's ASCII-only check, all relative source links
  resolve locally, and `git diff --check` reports no whitespace errors.
- No source code was changed as part of this audit.

## Recommended work order

1. Fix #181 before allowing untrusted builders to control zone names or using
   `export` on such zones.
2. Fix and regression-test #157 across all hostile entry points.
3. Complete #92 as a single index-formatting pass rather than patching isolated
   output lines.
4. Fix #57 or deliberately remove the unsupported fallback implementation.
5. Decide and document the builder/player compatibility policies behind #47,
   #86, and the #105/#147 ordering tradeoff.

## Audit limits

- This is a source-level applicability audit, not a general vulnerability
  assessment. It does not claim that unrelated defects are absent.
- Closed upstream issues were not assumed fixed locally; each code-bearing
  report was checked against current LuminariMUD source.
- For broad or unreproducible support reports, the disposition applies to the
  concrete failure and code sites described in the issue, not every possible
  bug in the named subsystem.
- The legal entry records repository state and provenance concerns only. It is
  not legal advice.
