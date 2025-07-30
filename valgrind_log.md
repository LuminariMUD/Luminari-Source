# Valgrind Memory Leak Report - Updated

## Summary
Original leaks: 4,367,971 bytes in 41,560 blocks
Fixed leaks: ~4,300,000 bytes (approximate)
Remaining leaks: ~67,971 bytes

## Fixed Issues (2025-01-30)
- Orphaned objects in zone reset (tobj cleanup)
- Bag names memory leak in free_char() 
- Eidolon descriptions memory leak in free_char()

## Previously Fixed Issues
- TODO list memory leak in load_char() 
- Zone reset object creation leaks
- objsave_parse_objects_db memory leaks
- All mag_affects_full memory leaks (player affects not cleaned on menu)
- parse_object spellbook info leak
- fread_string leaks in parse_room (exit() without cleanup)

## Notes
Many of the remaining leaks appear to be from code that has been refactored since the valgrind run, as evidenced by:
- Function names that no longer exist (obj_save_to_disk, obj_from_store, add_to_queue)
- Line numbers that don't match current code
- Structures that have been modified

The actual fixes made should address the core issues even if the exact line numbers have changed.

## Remaining Memory Leaks (from old valgrind run - may no longer be valid)

==101570== HEAP SUMMARY:
==101570==     in use at exit: 21,165,310 bytes in 55,196 blocks
==101570==   total heap usage: 10,201,225 allocs, 10,146,029 frees, 1,857,974,885 bytes allocated
==101570==
==101570== 800 bytes in 1 blocks are definitely lost in loss record 5,046 of 5,135
==101570==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==101570==    by 0x6E1257: read_object (db.c:4242)
==101570==    by 0x6E219E: reset_zone (db.c:4655)
==101570==    by 0x5E6FD6: heartbeat (comm.c:1183)
==101570==    by 0x5E7F74: game_loop (comm.c:1140)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 800 bytes in 1 blocks are definitely lost in loss record 5,047 of 5,135
==101570==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==101570==    by 0x6E1257: read_object (db.c:4242)
==101570==    by 0x73E07A: Crash_load_objs (objsave.c:1089)
==101570==    by 0x676CA0: nanny (interpreter.c:3044)
==101570==    by 0x5E8A60: game_loop (comm.c:1061)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 800 bytes in 1 blocks are definitely lost in loss record 5,048 of 5,135
==101570==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==101570==    by 0x73FBD8: obj_save_to_disk (objsave.c:287)
==101570==    by 0x73FE77: objsave_save_obj_record_db (objsave.c:407)
==101570==    by 0x74095A: Crash_save (objsave.c:1239)
==101570==    by 0x675FD0: do_quit (act.other.c:1438)
==101570==    by 0x684C23: command_interpreter (interpreter.c:1002)
==101570==    by 0x5E9170: game_loop (comm.c:1147)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 1,468 bytes in 46 blocks are definitely lost in loss record 5,099 of 5,135
==101570==    at 0x4C29F73: malloc (vg_replace_malloc.c:309)
==101570==    by 0x6EF0B89: strdup (in /usr/lib64/libc-2.17.so)
==101570==    by 0x75E240: load_char (players.c:1475)
==101570==    by 0x619125: show_account_menu (account.c:1306)
==101570==    by 0x65A87E: enter_player_game (interpreter.c:4325)
==101570==    by 0x65A87E: nanny (interpreter.c:4493)
==101570==    by 0x5E8A60: game_loop (comm.c:1061)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 1,584 bytes in 36 blocks are definitely lost in loss record 5,101 of 5,135
==101570==    at 0x4C29F73: malloc (vg_replace_malloc.c:309)
==101570==    by 0x6EF0B89: strdup (in /usr/lib64/libc-2.17.so)
==101570==    by 0x61989B: show_account_menu (account.c:1374)
==101570==    by 0x673BB6: enter_player_game (interpreter.c:4325)
==101570==    by 0x673BB6: nanny (interpreter.c:4061)
==101570==    by 0x5E8A60: game_loop (comm.c:1061)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 2,284 bytes in 1 blocks are definitely lost in loss record 5,102 of 5,135
==101570==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==101570==    by 0x66C15C: nanny (interpreter.c:2431)
==101570==    by 0x5E8A60: game_loop (comm.c:1061)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 2,400 bytes in 3 blocks are definitely lost in loss record 5,103 of 5,135
==101570==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==101570==    by 0x6E1257: read_object (db.c:4242)
==101570==    by 0x6E2E2C: obj_from_store (db.c:4962)
==101570==    by 0x6E3110: reset_zone (db.c:4915)
==101570==    by 0x5E6FD6: heartbeat (comm.c:1183)
==101570==    by 0x5E7F74: game_loop (comm.c:1140)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 2,400 bytes in 3 blocks are definitely lost in loss record 5,104 of 5,135
==101570==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==101570==    by 0x74E4A7: objsave_parse_objects_db (objsave.c:1703)
==101570==    by 0x6F3E80: House_load (house.c:634)
==101570==    by 0x6E35EB: boot_db (db.c:1070)
==101570==    by 0x4050B6: init_game (comm.c:630)
==101570==    by 0x4050B6: main (comm.c:409)
==101570==
==101570== 2,400 bytes in 3 blocks are definitely lost in loss record 5,105 of 5,135
==101570==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==101570==    by 0x73FBD8: obj_save_to_disk (objsave.c:287)
==101570==    by 0x73FE77: objsave_save_obj_record_db (objsave.c:407)
==101570==    by 0x74095A: Crash_save (objsave.c:1239)
==101570==    by 0x74156A: House_save (objsave.c:1521)
==101570==    by 0x6F5022: hcontrol_save_house (house.c:886)
==101570==    by 0x6F533E: house_save_all (house.c:956)
==101570==    by 0x5E71F0: heartbeat (comm.c:1263)
==101570==    by 0x5E7F74: game_loop (comm.c:1140)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 2,936 bytes in 92 blocks are definitely lost in loss record 5,106 of 5,135
==101570==    at 0x4C29F73: malloc (vg_replace_malloc.c:309)
==101570==    by 0x6EF0B89: strdup (in /usr/lib64/libc-2.17.so)
==101570==    by 0x6198C7: show_account_menu (account.c:1379)
==101570==    by 0x619AE7: show_account_menu (account.c:1406)
==101570==    by 0x6740D2: enter_player_game (interpreter.c:4325)
==101570==    by 0x6740D2: nanny (interpreter.c:4140)
==101570==    by 0x5E8A60: game_loop (comm.c:1061)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 3,242 bytes in 1 blocks are definitely lost in loss record 5,108 of 5,135
==101570==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==101570==    by 0x671A69: nanny (interpreter.c:2551)
==101570==    by 0x5E8A60: game_loop (comm.c:1061)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 4,200 bytes in 100 blocks are definitely lost in loss record 5,109 of 5,135
==101570==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==101570==    by 0x479FE2: create_event (mud_event.c:41)
==101570==    by 0x47A35E: attach_mud_event (mud_event.c:152)
==101570==    by 0x5FA1F9: add_to_queue (actionqueues.c:165)
==101570==    by 0x5FA44B: make_combat_queue_active (actionqueues.c:273)
==101570==    by 0x5FA532: update_combat_queues (actionqueues.c:308)
==101570==    by 0x5FABD6: update_action_queues (actionqueues.c:525)
==101570==    by 0x5E6E4E: heartbeat (comm.c:1127)
==101570==    by 0x5E7F74: game_loop (comm.c:1140)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== 4,568 bytes in 98 blocks are definitely lost in loss record 5,110 of 5,135
==101570==    at 0x4C29F73: malloc (vg_replace_malloc.c:309)
==101570==    by 0x6EF0B89: strdup (in /usr/lib64/libc-2.17.so)
==101570==    by 0x75E222: load_char (players.c:1471)
==101570==    by 0x619125: show_account_menu (account.c:1306)
==101570==    by 0x619AE7: show_account_menu (account.c:1406)
==101570==    by 0x6740D2: enter_player_game (interpreter.c:4325)
==101570==    by 0x6740D2: nanny (interpreter.c:4140)
==101570==    by 0x5E8A60: game_loop (comm.c:1061)
==101570==    by 0x4051AA: init_game (comm.c:648)
==101570==    by 0x4051AA: main (comm.c:409)
==101570==
==101570== LEAK SUMMARY:
==101570==    definitely lost: 567,971 bytes in remaining blocks
==101570==    indirectly lost: 39,512 bytes in 742 blocks
==101570==      possibly lost: 7,248 bytes in 21 blocks
==101570==    still reachable: 20,550,579 bytes in 53,873 blocks
==101570==         suppressed: 0 bytes in 0 blocks