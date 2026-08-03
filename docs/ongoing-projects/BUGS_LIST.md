# Verified Local Code Bug Backlog

This file contains bugs whose fixes belong in the dev/local source tree. Any
item requiring a change under `lib/`, in a help file, or in production help
data is tracked separately in [LIB-BUGS.md](LIB-BUGS.md).

The backlog was reviewed against source revision `ff3bf9e9` on 2026-08-03,
then split by fix ownership. No verified dev/local source-code items remain.
`LIB-BUGS.md` contains 14 production-owned items drawn from 17 records.

Across both files, 17 unique records remain from the original 146-record
snapshot. The other 129 were removed because they are resolved, intentional,
contradicted by current code or data, or cannot be tied to a current defect.

This was a source-based review rather than a live reproduction pass. Reproduce
each symptom before implementing its fix. Reporter metadata uses `L` for
character level, `R` for room VNUM, and `#` for the record's position in the
2026-08-03 production snapshot.

## Current status

No verified local code bugs remain. Production-owned data and help work stays
in [LIB-BUGS.md](LIB-BUGS.md).
