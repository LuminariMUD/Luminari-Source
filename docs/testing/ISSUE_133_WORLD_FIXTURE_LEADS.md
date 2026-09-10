# Issue 133 world-fixture lead dispositions

Historical event-core baseline and wakeup notes recorded four development-content
leads. They are not production defects. This note records the current package,
revision, and `wtool` result for each item. No replacement rooms, mobiles,
objects, or triggers were manufactured to silence the historical text.

Base inventory: `origin/master` `14dbdb969`. Tracked fixture correction is in
this change. Validator of record: `python3 scripts/world/wtool.py`. Protected
headers were not edited. Issue #123 is out of scope.

Tracked supported packages are `lib/world/minimal` and `lib/world/artifacts`.
Those bundles are flat directories (`index.mob` beside the data files), so
validation uses the same entry point as the world-tool graph test:

```sh
python3 scripts/world/wtool.py validate --paths lib/world/minimal
python3 scripts/world/wtool.py validate --paths lib/world/artifacts
```

`wtool show` with `--world-root lib/world/minimal` looks for `mob/index`, not
this flat layout, and is not used as the gate for these bundles.

Local OLC under `lib/world/{mob,obj,wld,zon,trg,...}` is gitignored and is not
part of this change.

## 16500-range wakeup golem

- Package: tracked `lib/world/minimal/16.mob` (indexed by `index.mob`).
- Absent from `lib/world/artifacts`. Local development `lib/world/mob/165.mob`
  starts at animal prototypes `16501+` and has no mobile `16500`; that live
  range was not overwritten.
- Before this change, current and default position values included out-of-range
  `10` and `POS_FIGHTING` (`8`). The wakeup visibility test had to `force golem
  stand`, which took the "stops floating around" default path.
- Intermediate `$` terminators after each record meant the parser and C boot
  loaded only `16500`. Those extra terminators were removed so `16501`-`16511`
  remain loadable records in the same file. One `$` remains at end of file.
- After this change every remaining `16500`-`16511` record uses `POS_STANDING`
  for current and default position (`9 9 1`).
- Validator: `validate --paths lib/world/minimal` reports no `MOB016` error.
  Direct `parse_mobile_file()` of `16.mob` loads vnums `16500`-`16511` with
  in-range standing positions.

## Zone 23 missing DG trigger references

- Absent from tracked `lib/world/minimal` and `lib/world/artifacts`
  (`show zone 23` is not found).
- Present in gitignored local `lib/world/zon/23.zon` (package 23, "3.5E testing
  zone"). Room attachments `T 2304`, `2305`, `2306`, `2307`, `2309`, and `2312`
  exist in `lib/world/trg/23.trg`.
- Validator: `wtool --world-root lib/world show/refs zone 23` and
  `validate --zone 23` report no missing trigger references and no errors.
  Remaining warnings (`SEM008`, `SEM009`, `REF025`) are unrelated to the named
  lead and were left alone.
- Disposition: closed as already repaired in local OLC; not in the supported
  tracked packages.

## Zone 1204 equipping object 120602

- Absent from tracked supported packages.
- Present in gitignored local `lib/world/obj/1204.obj` (object `120602`,
  "obsidan broadsword") and `lib/world/zon/1204.zon` (`E` resets at lines 272
  and 274).
- Validator: `show`/`refs` find the object; incoming refs are the two `E`
  resets. `validate --zone 1204` reports no missing-object references, no
  `REF030` wear mismatch for `120602`, and no errors.
- Disposition: closed as already repaired in local OLC; not in the supported
  tracked packages.

## Mobile 200103 SPEC flag without a procedure

- Absent from tracked supported packages.
- Present in gitignored local `lib/world/mob/2001.mob` as Brother Spire
  (package 2001). Action flags `253962` decode to sentinel/ISNPC/immunities
  and do not include `MOB_SPEC`. `spec_proc` is unset.
- Validator: `show`/`refs` find the mobile. `validate --zone 2001` reports no
  SPEC-without-procedure finding and no errors. One unrelated warning,
  `SEM012`, notes the level-34 mobile is outside zone 2001's 1..30 band.
- Disposition: closed as already repaired in local OLC (SPEC flag absent);
  not in the supported tracked packages. No procedure was invented.

## Boot-log corroboration

No live `luminari` process was running during this inventory. Retained
development `log/syslog*` and `log/errors` from 2026-09-05 through 2026-09-09
contain no `MOB ERROR`, no `SPEC flag` complaints, and no `ZONE ERROR` lines
for 23 / 1204 / 120602. Optional Ollama/I3 noise is ignored.
