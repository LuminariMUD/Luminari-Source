# Durable Binary File Formats

The server keeps three binary files under `lib/etc/`: the legacy bulletin boards, the house control
file, and the retired login log. Until issue #95 each was a native C structure copied to disk with
`fwrite()`, so its meaning depended on the compiler's padding, integer widths, byte order, and even
pointer values. This document specifies the portable formats that replaced them, how the server
reads the old files, and the rules for changing a format.

`src/core/binary_formats.c` owns every encoder and decoder. It has no game dependencies, so the same
code runs in the server, in the focused test harness, and under the fuzzer. The file owners call it:
`src/comms/boards.c`, `src/obj/house.c`, and `src/act/act.wizard.c`. The shared load and save helpers
(`read_durable_file()`, `replace_durable_file()`, `quarantine_durable_file()`) are in
`src/core/utils.c`.

## Inventory

Every project-owned `fread()`/`fwrite()` path, classified when issue #95 was fixed:

| Owner | File | Class |
| -- | -- | -- |
| `src/comms/boards.c` | `lib/etc/board.general`, `lib/etc/board.immortal` | Encoded binary, `LMBD` v1; reads legacy |
| `src/obj/house.c` | `lib/etc/hcontrol` | Encoded binary, `LMHC` v1; reads legacy |
| `src/act/act.wizard.c` | `lib/etc/last` | Legacy layout, read only by `last all` |
| `src/obj/house.c`, `src/obj/objsave.c` | `lib/house/*.house` | Text object records |
| `src/player/players.c` | `lib/plrfiles/*/*.plr` | Text, written from a byte buffer |
| `src/player/player_rename.c`, `src/olc/hedit.c` | Rename snapshots, help file copies | Byte copies |
| `src/core/perfmon.c`, `src/net/i3_client.c` | Perfmon snapshot, I3 configuration | Text |
| `src/wilderness/wilderness.c` | Wilderness map images | PNG written by libgd |

Two native-layout readers were removed rather than ported, because no data in their formats remains.
`hcontrol asciiconvert` read CircleMUD 3.1 binary rent files; house contents have been text records
and database rows since that conversion, and run on those text files the command dereferenced a
NULL object. `util/plrtoascii` converted a CircleMUD binary player file; players load from ASCII files and
MariaDB. The login log writer had returned before writing anything since 2022 and was deleted with
its boot-time trim; the `last` command reads MariaDB.

## Common envelope

Every integer is little-endian two's complement, whatever the host. A current file is a 16-byte
envelope followed by the payload:

| Offset | Size | Field |
| -- | -- | -- |
| 0 | 4 | Magic: `LMBD` (boards) or `LMHC` (house control) |
| 4 | 2 | Format version, u16 |
| 6 | 2 | Byte-order mark 0xFEFF, u16 (bytes `FF FE`) |
| 8 | 4 | Payload size in bytes, u32 |
| 12 | 4 | CRC-32 of the payload, u32: ISO-HDLC, as zlib's `crc32()` |
| 16 | n | Payload |

A string is a u32 size followed by that many bytes. The size counts the terminating NUL, which must
be the last byte and the only NUL; size 0 means the string is absent (NULL).

A decoder rejects, in this order: a file that does not start with the magic (it is then read as the
legacy layout), a wrong byte-order mark, a version it does not know, a payload size that does not
match the file size, a checksum mismatch, a count or size over its limit, a count the remaining bytes
cannot hold (checked before allocating), a malformed string, and any bytes left after the last
record. It decodes into new memory and hands it over only when the whole file is valid, so a
rejected file changes nothing. An empty file is a legacy file with no records.

The formats have a fixed schema per version, so a field cannot be duplicated or unknown; the version
number is the only feature identifier.

## Board file, version 1 (`LMBD`)

| Payload field | Type |
| -- | -- |
| Post count | u32, at most `MAX_BOARD_MESSAGES` (300) |
| For each post: poster level | i32 |
| For each post: heading | string, at most 1024 bytes |
| For each post: message body | string, at most 49152 bytes |

Posts are stored in board order. `board_save_board()` writes an empty board as a count of 0; it no
longer deletes the file.

## House control file, version 1 (`LMHC`)

| Payload field | Type |
| -- | -- |
| House count | u32, at most `MAX_HOUSES` (999) |
| For each house: vnum, atrium | u32, u32 |
| For each house: exit direction | i16 |
| For each house: ownership mode | i32 |
| For each house: built, owner, last payment, flags, builder | i64 each |
| For each house: guest count | u32, at most 99 |
| For each house: guest ids | i64 each, guest count of them |

`MAX_GUESTS` must equal `HOUSE_FILE_MAX_GUESTS` (99); `house.c` enforces this at compile time.
`House_boot()` still skips records whose owner, rooms, exit, or mode are invalid, and the save at the
end of boot drops them.

## Legacy layouts (read only)

The legacy layouts are the x86-64 (LP64) structures the server wrote before issue #95. They are
frozen: decoders read them field by field, and nothing writes them.

Board file: an i32 post count (0-300), then per post a 32-byte record followed by the heading bytes
and the message bytes. In the record, offset 16 is the level (i32), 20 the heading size (i32, 1-1024,
terminator included), and 24 the message size (i32, 0 for none). Offsets 0 (slot number) and 8
(heading pointer) held runtime values and are ignored.

House control file: 912-byte records with nothing between them; a partial record is rejected. Offsets:
0 vnum (u32), 4 atrium (u32), 8 exit (i16), 16 built (i64), 24 mode (i32), 32 owner (i64), 40 guest
count (i32, 0-99), 48 guests (99 x i64), 840 last payment (i64), 848 flags (i64), 856 builder (i64).
Guest slots past the count held stale values and are not read. The six spare fields at 864-911 were
never used.

Login log: 304-byte records. Offsets: 0 close type (i32), 4 host name (256 bytes), 260 user name
(16 bytes, not always terminated), 280 login time (i64), 288 close time (i64), 296 id (i32), 300
connection id (i32). `last all` refuses a file whose size is not a whole number of records.

## Saving and recovery

A save encodes the data, writes it to `<file>.tmp`, flushes, fsyncs, closes, and renames the
temporary file over the live one (`finish_file_save()`). A crash or a failed write leaves the live
file untouched; the next save overwrites the stale temporary file.

When the file being replaced is not in the current format, the save first copies it to
`<file>.legacy-<crc32>`, where the suffix is the CRC-32 of its contents in hex. Because the name
follows the contents, repeating the copy rewrites the same bytes, so repeated saves leave one backup.
If the backup cannot be written, the save is refused and the live file is kept.

When a file cannot be read or decoded, the server logs a SYSERR naming the reason and renames the
file to `<file>.rejected-<unix time>-<process id>`, so no later save can overwrite it. The board, or the house
list, starts empty. `House_boot()` does not save after a rejection, so no empty control file replaces
the rejected one until a house changes. A file written by a newer server (an unknown version) is
rejected the same way. To recover, fix or replace the rejected file, move it back, and reboot.

## Upgrading and rolling back

The upgrade needs no separate tool. After deployment, `House_boot()` upgrades `hcontrol` at boot,
and each board is upgraded by its first save, a post or a removal. The loader logs each legacy file
it reads: the control file at boot, a board the first time someone uses it. The production run of
this change is a separate, reviewed operational step:

1. With the old server stopped, copy `lib/etc/board.*`, `lib/etc/hcontrol`, and `lib/etc/last`.
2. Start the new server. Check the log for the legacy-layout lines, any `Rejected` SYSERRs, and the
   `Preserved the legacy` lines naming each backup (a board's appear after its first post or
   removal).
3. Check that `hcontrol show` and the boards list what they did before.

To roll back, stop the server, move each `<file>.legacy-<crc32>` back over its `<file>`, and start
the previous binary. Changes saved after the upgrade are lost. Never start an older binary on
current-format files: its board loader deletes a file it cannot read, and its house boot discards
every house.

## Changing a format

- Any change an older reader would misread (a new field, a new meaning, a larger limit) needs a new
  version number. Version numbers are never reused.
- The encoder writes only the newest version. Keep a decoder for every version that may exist on
  disk, and add a golden fixture for the new version beside the existing ones.
- A decoder that meets a newer version rejects the file (see above). It never guesses.
- A new durable binary file gets its own magic, a section here, a codec in `binary_formats.c`, golden
  fixtures, tests, and a fuzzer selector.

## Verification

- `make test` runs `unittests/CuTest/test_binary_formats.c` (golden legacy and current files, every
  truncation, every single-bit corruption, malformed fields with valid checksums) and
  `unittests/CuTest/test_binary_file_persistence.c` (upgrade and backup, rejection, a save killed
  mid-write by `RLIMIT_FSIZE`, and `last all`).
- `make -C unittests/CuTest binary-formats` runs the codec tests alone. It needs no configured build,
  so CI also runs it on AArch64 (the `Binary file formats (AArch64)` job).
- `make -C unittests/CuTest binary-formats-fuzz FUZZ_SECONDS=15` fuzzes every decoder with ASan and
  UBSan; the sanitizer CI job runs it. A decoded file must re-encode and decode to the same content.
  Seeds are hex text files in `unittests/CuTest/fuzz_corpus_binary_formats/`. Add every input that ever
  crashed a decoder there and as a fixed case in `test_binary_formats.c`.
- `scripts/ci/check_native_struct_io.py` (run by `make test` and CTest) fails on any `fread()` or
  `fwrite()` of elements larger than a byte, and any `read()`/`write()` of `sizeof(struct ...)`,
  under `src/` and `util/`. There is no baseline, so a new native-structure file fails review.
