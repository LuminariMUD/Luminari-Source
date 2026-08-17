# Autostat and Tier Damage Assessment

Status: corrected and verified in development on 2026-08-17.

## Required behavior

The pre-tier autostat at `0c94373d` remains the base. Tier is only an input to
autostat:

1. Calculate the complete existing level, class, race, reward, HP, loader, and combat
   behavior.
2. Standard or an unspecified tier adds nothing.
3. A higher tier adds HP, hitroll, armor, and damroll to the completed base.
4. Save those ordinary fields. Changing `Tier:` later has no effect until autostat is
   run again.

The loader and combat system never read Tier to add live bonuses.

## Damage found

The regression entered in `16ee5127` and expanded in `5975766c`:

- replaced `autoroll_mob()` instead of building on it;
- removed the level-31-to-34 base HP, damage, experience, and gold block;
- changed variable prototype HP from `1dY+H` to fixed `1d1+(H-1)`;
- replaced existing loader and combat effects with live Tier checks;
- changed Psionicist PSP, Giant size, StrAdd, spell-resistance, Sorcerer/Bard category,
  and critical-immunity boundary behavior; and
- encoded the replacement behavior into its regression tests and all 12,407
  converted RoL mobiles across 222 files.

## Correction

- Restored the original production autostat, loader, and level-based combat paths.
- Added one post-base helper for Tier bonuses to saved HP, hitroll, armor, and
  damroll. It is called only when autostat runs.
- Restored Standard and unspecified as strict no-ops.
- Restored variable HP serialization for ordinary generated mobiles.
- Removed live Tier-derived attacks, extra attacks, critical effects, defense bypass,
  armor, and loader HP behavior.
- Updated MEDIT and `stat` to state that Tier is metadata until autostat is rerun.
- Regenerated and applied all 12,407 RoL mobiles from the sealed source lineage.

## Verification

- Production-linked CuTest: 764 passed.
- World-tool tests: 428 passed.
- Two complete Phase 7 generations were byte-identical.
- Phase 8 release `rol-phase8-release-efa9226a40ac0cb0` passed conversion,
  preservation, runtime-contract, namespace, persistence, isolation, and boot gates.
- The bounded candidate boot entered the game loop, reset zone `#20000`, terminated
  normally, and reported no converted-VNUM diagnostics.
- Applying the release changed exactly the 222 generated mobile files; 984 other
  candidate paths were already current.
- Post-apply validation matched the candidate and repeat application was a no-op.
