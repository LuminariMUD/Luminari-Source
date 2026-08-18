-- Correct the ARMOR section of the oedit-values help entry.
--
-- The old text described a per-slot multiplier (Body X3, Head and Legs X2)
-- that the code has never applied: apply_ac() in src/handler.c uses a factor
-- of 1. It also left the scale of value 0 unstated. Armor class from value 0
-- is now gated to the five real armor slots, matching the slots that carry
-- armor check penalty, spell failure, and the max-dex cap.
--
-- Idempotent: the REPLACE() is a no-op once the new text is in place.
-- Mirrors the same correction made to lib/text/help/help.hlp.

UPDATE help_entries
SET entry = REPLACE(
      entry,
'   value 0: AC-apply of the armor.  Note that the effective change to AC is
this value times a multiplier based on where the armor is worn (Body X3,
Head and Legs X2). Positive values enhance AC; negative values hurt AC
(cursed armor for example).
',
'   value 0: AC-apply of the armor, stated in tenths of an armor-class point,
so a value of 50 is 5.0 AC. Positive values enhance AC; negative values hurt
AC (cursed armor for example).
   value 0 only grants armor class when the piece is worn on one of the five
armor slots: SHIELD, HEAD, BODY, ARMS, or LEGS. Worn anywhere else it is
ignored, because value 0 means something different for every other item type.
To give armor class to any other slot, use an APPLY_AC_NEW affection instead
of value 0. The same five slots are the ones that carry armor check penalty,
arcane spell failure, and the max-dex cap.
')
WHERE tag = 'oedit-values';
