==32435== Memcheck, a memory error detector
==32435== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==32435== Using Valgrind-3.15.0 and LibVEX; rerun with -h for copyright info
==32435== Command: bin/circle 4101
==32435== 
Jul 26 14:54:18 :: Loading configuration.
Jul 26 14:54:18 :: LuminariMUD 2.4839 (tbaMUD 3.64)
Make time: Sat Jul 26 14:50:26 UTC 2025
Make user: zusuk
Make host: None
Branch: * master
Parent: fd4115fd6400a70ae33135711dec58dac56efb35

Jul 26 14:54:18 :: Using lib as data directory.
Jul 26 14:54:18 :: Running game on port 4101.
Jul 26 14:54:18 :: Finding player limit.
Jul 26 14:54:18 ::    Setting player limit to 300 using rlimit.
Jul 26 14:54:18 :: Opening mother connection.
Jul 26 14:54:18 :: Binding to all IP interfaces on this host.
Jul 26 14:54:18 :: Boot db -- BEGIN.
Jul 26 14:54:18 :: Resetting the game time:
Jul 26 14:54:18 ::    Current Gametime: 20H 25D 13M 1004Y.
Jul 26 14:54:18 :: Initialize Global / Group Lists
Jul 26 14:54:18 :: Initializing Events
Jul 26 14:54:18 :: Reading news, credits, help, ihelp, bground, info & motds.
Jul 26 14:54:18 :: Loading spell definitions.
Jul 26 14:54:18 :: Loading weapon and armor special ability definitions.
Jul 26 14:54:18 :: SUCCESS: Connected to MySQL database 'luminari_muddev' on host 'localhost' as user 'luminari_mud'
Jul 26 14:54:18 :: Loading zone table.
Jul 26 14:54:18 ::    514 zones, 45232 bytes.
Jul 26 14:54:19 :: Loading triggers and generating index.
Jul 26 14:54:19 :: Loading rooms.
Jul 26 14:54:20 ::    50233 rooms, 11654056 bytes.
Jul 26 14:54:21 :: SYSERR: dg_read_trigger: Trigger vnum #2315 asked for but non-existant! (room:-1)
Jul 26 14:54:38 :: Loading regions. (MySQL)
Jul 26 14:54:38 :: INFO: Loading region data from MySQL
Jul 26 14:54:38 ::  adding event for vnum 1000012
Jul 26 14:54:38 :: TEST DEBUG REGION EVENTS: vnum 1000012 rnum 11
Jul 26 14:54:38 :: Loading paths. (MySQL)
Jul 26 14:54:38 :: INFO: Loading path data from MySQL
Jul 26 14:54:38 :: Renumbering rooms.
Jul 26 14:54:38 :: Checking start rooms.
Jul 26 14:54:38 :: Loading mobs and generating index.
Jul 26 14:54:39 ::    14563 mobs, 466016 bytes in index, 260502944 bytes in prototypes.
Jul 26 14:54:48 :: SYSERR: dg_read_trigger: Trigger vnum #2314 asked for but non-existant! (mob: the Dark Knight - -1)
Jul 26 14:54:48 :: SYSERR: dg_read_trigger: Trigger vnum #2316 asked for but non-existant! (mob: the Dark Knight - -1)
Jul 26 14:54:48 :: SYSERR: dg_read_trigger: Trigger vnum #2310 asked for but non-existant! (mob: a giant mother spider - -1)
Jul 26 14:54:48 :: SYSERR: dg_read_trigger: Trigger vnum #2313 asked for but non-existant! (mob: a bat-like creature - -1)
Jul 26 14:58:35 :: Loading objs and generating index.
Jul 26 14:58:35 ::    12160 objs, 389120 bytes in index, 7587840 bytes in prototypes.
Jul 26 14:58:35 :: SYSERR: Trigger vnum #2308 asked for but non-existant! (Object: a jet black pearl - -1)
Jul 26 14:58:35 :: SYSERR: Trigger vnum #2311 asked for but non-existant! (Object: a large stone chest - -1)
Jul 26 14:58:35 :: SYSERR: Trigger vnum #2317 asked for but non-existant! (Object: Helm of Brilliance - -1)
Jul 26 14:58:39 :: Renumbering zone table.
Jul 26 14:58:39 :: SYSERR: zone file: Invalid vnum 15802, cmd disabled
Jul 26 14:58:39 :: SYSERR: ...offending cmd: 'O' cmd in zone #158, line 8
Jul 26 14:58:39 :: Loading shops.
Jul 26 14:58:39 :: Placing Harvesting Nodes
Jul 26 14:58:40 :: Loading quests.
Jul 26 14:58:40 ::    169 entries, 29744 bytes.
Jul 26 14:58:40 :: Loading Homeland quests.
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: SYSERR: Invalid quest command type 'S' in quest file
Jul 26 14:58:40 :: Loading Deities
Jul 26 14:58:40 :: Loading Domains.
Jul 26 14:58:40 :: Loading Weapons.
Jul 26 14:58:40 :: Loading Armor.
Jul 26 14:58:40 :: Loading Extended Races
Jul 26 14:58:40 :: Loading feats.
Jul 26 14:58:40 :: Loading Class List
Jul 26 14:58:40 :: Loading evolutions.
Jul 26 14:58:40 :: Object (V) 19216 does not exist in database.
Jul 26 14:58:40 :: Object (V) 19216 does not exist in database.
Jul 26 14:58:41 :: Initializing perlin noise generator.
Jul 26 14:58:41 :: Indexing wilderness rooms.
Jul 26 14:58:41 :: Writing wilderness map image.
Jul 26 14:58:41 :: Loading help entries.
Jul 26 14:58:41 ::    3279 entries, 104928 bytes.
Jul 26 14:58:45 :: Generating player index.
Jul 26 14:58:45 :: Loading fight messages.
Jul 26 14:58:45 :: Loaded 163 Combat Messages...
Jul 26 14:58:45 :: Loading social messages.
Jul 26 14:58:45 :: Social table contains 501 socials.
Jul 26 14:58:46 :: Building command list.
Jul 26 14:58:46 :: Command info rebuilt, 1249 total commands.
Jul 26 14:58:46 :: Assigning function pointers:
Jul 26 14:58:46 ::    Mobiles.
Jul 26 14:58:46 ::    Shopkeepers.
Jul 26 14:58:46 ::    Objects.
Jul 26 14:58:46 ::    Rooms.
Jul 26 14:58:46 ::    Questmasters.
Jul 26 14:58:46 :: SYSERR: Quest #0 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #2011 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20309 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20315 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20316 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20317 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20318 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20319 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20320 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20321 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20322 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20323 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20324 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #20325 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #102412 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #102413 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #102414 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #102415 has no questmaster specified.
Jul 26 14:58:46 :: SYSERR: Quest #128102 has no questmaster specified.
Jul 26 14:58:46 :: Assigning spell and skill levels.
Jul 26 14:58:46 :: Sorting command list...
Jul 26 14:58:46 :: Sorting spells/skills...
Jul 26 14:58:46 :: Booting mail system.
Jul 26 14:58:46 ::    Mail file read -- 46 messages.
Jul 26 14:58:46 :: Reading banned site and invalid-name list.
Jul 26 14:58:46 :: Loading Ideas.
Jul 26 14:58:46 :: Loading Bugs.
Jul 26 14:58:46 :: Loading Typos.
Jul 26 14:58:46 :: Loading random encounter tables.
Jul 26 14:58:46 :: Loading hunts table.
Jul 26 14:58:46 :: Spawning hunts for the first time this boot.
Jul 26 14:58:46 :: Deleting timed-out crash and rent files:
Jul 26 14:58:46 :: debug: deleting crash file plrobjs/P-T/phaelae.objs
Jul 26 14:58:46 ::     Deleting phaelae's crash file.
Jul 26 14:58:46 ::    Done.
Jul 26 14:58:46 :: Booting crafts.
Jul 26 14:58:46 :: Booting clans.
Jul 26 14:58:46 :: Loading clan zone claim info.
Jul 26 14:58:46 ::    Clan file does not exist. Will create a new one
Jul 26 14:58:46 :: Cleaning up last log.
Jul 26 14:58:46 :: Resetting #0: <*> Builder Academy Zone (rooms 0-99).
Jul 26 14:58:46 :: Resetting #1: <*> Sanctus (rooms 100-199).
Jul 26 14:58:46 :: Resetting #2: <*> Sanctus II (rooms 200-299).
Jul 26 14:58:47 :: Resetting #3: <*> Sanctus III (rooms 300-399).
Jul 26 14:58:47 :: Resetting #4: Jade Forest (rooms 400-499).
Jul 26 14:58:47 :: Resetting #5: Newbie Farm (rooms 500-599).
Jul 26 14:58:47 :: Resetting #6: Sea of Souls (rooms 600-699).
Jul 26 14:58:47 :: Resetting #7: Camelot (rooms 700-799).
Jul 26 14:58:47 :: Resetting #8: <*> Houses / Internal (rooms 800-899).
Jul 26 14:58:47 :: Resetting #9: River Island of Minos (rooms 900-999).
Jul 26 14:58:47 :: Resetting #10: Orme's Script Test Zone (rooms 1000-1099).
Jul 26 14:58:47 :: Resetting #11: Frozen Castle (rooms 1100-1199).
Jul 26 14:58:47 :: Resetting #12: <*> Staff Simplex (rooms 1200-1299).
Jul 26 14:58:47 :: Resetting #13: <*> Luminari Examples (rooms 1300-1399).
Jul 26 14:58:47 :: Resetting #14: <*> Luminari Examples II (rooms 1400-1499).
Jul 26 14:58:47 :: Resetting #15: Straight Path (rooms 1500-1599).
Jul 26 14:58:47 :: Resetting #16: Camelot II (rooms 1600-1699).
Jul 26 14:58:47 :: Resetting #17: Camelot III (rooms 1700-1799).
Jul 26 14:58:47 :: Resetting #18: War Torn Wasteland (rooms 1800-1899).
Jul 26 14:58:47 :: Resetting #19: Spider Swamp (rooms 1900-1999).
Jul 26 14:58:47 :: Resetting #20: Untitled (Arena) (rooms 2000-2099).
Jul 26 14:58:47 :: Resetting #21: Hobbiton (rooms 2100-2199).
Jul 26 14:58:47 :: Resetting #22: Tower of the Undead (rooms 2200-2299).
Jul 26 14:58:47 :: Resetting #23: 3.5E testing zone (rooms 2300-2399).
Jul 26 14:58:47 :: Resetting #24: Acererak Zone (rooms 2400-2499).
Jul 26 14:58:47 :: Resetting #25: Shadow Grove (rooms 2500-2599).
Jul 26 14:58:47 :: Resetting #26: High Tower of Magic II (rooms 2600-2699).
Jul 26 14:58:47 :: Resetting #27: Memlin Caverns (rooms 2700-2799).
Jul 26 14:58:47 :: Resetting #28: Mudschool (rooms 2800-2899).
Jul 26 14:58:47 :: Resetting #29: Unname Zone (rooms 2900-2999).
Jul 26 14:58:47 :: Resetting #30: <*> Northern Midgen (rooms 3000-3099).
Jul 26 14:58:47 :: Resetting #31: <*> Southern Midgen (rooms 3100-3199).
Jul 26 14:58:47 :: Resetting #32: <*> Midgen (rooms 3200-3299).
Jul 26 14:58:47 :: Resetting #33: <*> Internal / Three of Swords (rooms 3300-3399).
Jul 26 14:58:47 :: Resetting #34: Nordmaar Keep (rooms 3400-3499).
Jul 26 14:58:47 :: Resetting #35: Miden'Nir (rooms 3500-3599).
Jul 26 14:58:47 :: Resetting #36: Chessboard of Midgen (rooms 3600-3699).
Jul 26 14:58:47 :: Resetting #37: Capital Sewer System (rooms 3700-3799).
Jul 26 14:58:47 :: Resetting #38: Capital Sewer System II (rooms 3800-3899).
Jul 26 14:58:47 :: Resetting #39: Haven (rooms 3900-3999).
Jul 26 14:58:48 :: Resetting #40: Mines of Moria (rooms 4000-4099).
Jul 26 14:58:48 :: Resetting #41: Mines of Moria (rooms 4100-4199).
Jul 26 14:58:48 :: Resetting #42: Dragon Chasm (rooms 4200-4299).
Jul 26 14:58:48 :: Resetting #43: Arctic Zone (rooms 4300-4399).
Jul 26 14:58:48 :: Resetting #44: Orc Camp (rooms 4400-4499).
Jul 26 14:58:48 :: Resetting #45: Woodland Monastery (rooms 4500-4599).
Jul 26 14:58:48 :: Resetting #46: Ant Hill (rooms 4600-4699).
Jul 26 14:58:48 :: Resetting #47: Valcrest I (rooms 4700-4799).
Jul 26 14:58:48 :: Resetting #48: Valcrest II (rooms 4800-4899).
Jul 26 14:58:48 :: Resetting #49: The Underworld (rooms 4900-4999).
Jul 26 14:58:48 :: Resetting #50: Great Eastern Desert (rooms 5000-5099).
Jul 26 14:58:48 :: Resetting #51: Drow City (rooms 5100-5199).
Jul 26 14:58:48 :: Resetting #52: City of Thalos (rooms 5200-5299).
Jul 26 14:58:48 :: Resetting #53: Great Pyramid (rooms 5300-5399).
Jul 26 14:58:48 :: Resetting #54: New Thalos (rooms 5400-5499).
Jul 26 14:58:48 :: Resetting #55: New Thalos II (rooms 5500-5599).
Jul 26 14:58:48 :: Resetting #56: New Thalos Wilderness (rooms 5600-5699).
Jul 26 14:58:48 :: Resetting #57: Untitled (Zodiac) (rooms 5700-5799).
Jul 26 14:58:48 :: Resetting #58: Cottage (rooms 5800-5899).
Jul 26 14:58:48 :: Resetting #59: Wizard Training Mansion (rooms 5900-5999).
Jul 26 14:58:49 :: Resetting #60: Haon-Dor, Light Forest (rooms 6000-6099).
Jul 26 14:58:49 :: Resetting #61: Haon-Dor, Light Forest II (rooms 6100-6199).
Jul 26 14:58:49 :: Resetting #62: Orc Enclave (rooms 6200-6299).
Jul 26 14:58:49 :: Resetting #63: Arachnos (rooms 6300-6399).
Jul 26 14:58:49 :: Resetting #64: Rand's Tower (rooms 6400-6499).
Jul 26 14:58:49 :: Resetting #65: Dwarven Kingdom (rooms 6500-6599).
Jul 26 14:58:49 :: Resetting #66: Faedon Forest (rooms 6600-6699).
Jul 26 14:58:49 :: Resetting #67: Graven Hollow Camp (rooms 6700-6799).
Jul 26 14:58:49 :: Resetting #68: New Zone for Anson (rooms 6800-6899).
Jul 26 14:58:49 :: Resetting #69: The Giant Darkwood Tree (rooms 6900-6999).
Jul 26 14:58:49 :: Resetting #70: Sewer, First Level (rooms 7000-7099).
Jul 26 14:58:49 :: Resetting #71: Second Sewer (rooms 7100-7199).
Jul 26 14:58:49 :: Resetting #72: Sewer Maze (rooms 7200-7299).
Jul 26 14:58:49 :: Resetting #73: Tunnels in the Sewer (rooms 7300-7399).
Jul 26 14:58:49 :: Resetting #74: Newbie Graveyard (rooms 7400-7499).
Jul 26 14:58:49 :: Resetting #75: Zamba (rooms 7500-7599).
Jul 26 14:58:49 :: Resetting #76: Grandmother's House (rooms 7600-7699).
Jul 26 14:58:49 :: Resetting #77: The Crystal Swamp (rooms 7700-7799).
Jul 26 14:58:49 :: Resetting #78: Gideon (rooms 7800-7899).
Jul 26 14:58:49 :: Resetting #79: Redferne's Residence (rooms 7900-7999).
Jul 26 14:58:49 :: Resetting #80: Maze of Madness (rooms 8000-8099).
Jul 26 14:58:49 :: Resetting #81: <*> Common Items/Mobs (rooms 8100-8299).
Jul 26 14:58:49 :: Resetting #83: Tomb of Horrors (rooms 8300-8399).
Jul 26 14:58:49 :: Resetting #84: Isis' New Zone (rooms 8400-8499).
Jul 26 14:58:49 :: Resetting #85: City of Delo (rooms 8500-8599).
Jul 26 14:58:49 :: Resetting #86: Duke Kalithorn's Keep (rooms 8600-8699).
Jul 26 14:58:49 :: Resetting #87: Glumgold's Sea (rooms 8700-8799).
Jul 26 14:58:49 :: Resetting #88: New Zone for Rosealee (rooms 8800-8899).
Jul 26 14:58:49 :: Resetting #89: Semada's unfinished zone (rooms 8900-8999).
Jul 26 14:58:49 :: Resetting #90: Oasis (rooms 9000-9099).
Jul 26 14:58:49 :: Resetting #91: The Orphanage (rooms 9100-9199).
Jul 26 14:58:49 :: Resetting #92: The Depths: Scorching Desert (rooms 9200-9299).
Jul 26 14:58:49 :: Resetting #93: New Zone for Ambanya (rooms 9300-9399).
Jul 26 14:58:49 :: Resetting #94: <*> Internal Use Only (rooms 9400-9599).
Jul 26 14:58:49 :: Resetting #96: Domiae (rooms 9600-9699).
Jul 26 14:58:49 :: Resetting #97: New Zone for Makiel (rooms 9700-9799).
Jul 26 14:58:49 :: Resetting #98: Radiant Heart Sanctuary (rooms 9800-9999).
Jul 26 14:58:49 :: Resetting #100: Northern Highway (rooms 10000-10099).
Jul 26 14:58:49 :: Resetting #101: Untitled (South Road) (rooms 10100-10199).
Jul 26 14:58:49 :: Resetting #102: Ratchet's Untitled Zone (rooms 10200-10299).
Jul 26 14:58:49 :: Resetting #103: Untitled (DBZ World) (rooms 10300-10399).
Jul 26 14:58:49 :: Resetting #104: Land of Orchan (rooms 10400-10499).
Jul 26 14:58:49 :: Resetting #105: The Pilgrimage of the Petitioners (rooms 10500-10599).
Jul 26 14:58:49 :: Resetting #106: Elcardo (rooms 10600-10699).
Jul 26 14:58:49 :: Resetting #107: Realms of Iuel (rooms 10700-10799).
Jul 26 14:58:49 :: Resetting #108: Wilderness Encounters (rooms 10800-10899).
Jul 26 14:58:49 :: Resetting #109: Hidden Campgrounds (rooms 10900-10999).
Jul 26 14:58:49 :: Resetting #110: Crystal City (rooms 11000-11099).
Jul 26 14:58:49 :: Resetting #111: Descent into Hell (rooms 11100-11199).
Jul 26 14:58:49 :: Resetting #113: The Jumping Slug (rooms 11300-11399).
Jul 26 14:58:49 :: Resetting #114: Glass Tower (rooms 11400-11499).
Jul 26 14:58:49 :: Resetting #115: Monestary Omega (rooms 11500-11599).
Jul 26 14:58:49 :: Resetting #116: Swinton Goldwater's Hideout (rooms 11600-11699).
Jul 26 14:58:49 :: Resetting #117: Los Torres (rooms 11700-11799).
Jul 26 14:58:49 :: Resetting #118: The Dollhouse (rooms 11800-11899).
Jul 26 14:58:49 :: Resetting #119: Arcanite Caves (rooms 11900-11999).
Jul 26 14:58:49 :: Resetting #120: Rome (rooms 12000-12099).
Jul 26 14:58:50 :: Resetting #121: Ant Caves (rooms 12100-12199).
Jul 26 14:58:50 :: Resetting #122: The Dreamscape (rooms 12200-12299).
Jul 26 14:58:50 :: Resetting #123: Rychus - New Zone (rooms 12300-12399).
Jul 26 14:58:50 :: Resetting #124: Bagoggle's Unnamed Zone (rooms 12400-12499).
Jul 26 14:58:50 :: Resetting #125: Hannah (rooms 12500-12599).
Jul 26 14:58:50 :: Resetting #126: Grammaton Fortress (rooms 12600-12699).
Jul 26 14:58:50 :: Resetting #128: English's Test Zone (rooms 12800-12899).
Jul 26 14:58:50 :: Resetting #130: Mist Maze (rooms 13000-13099).
Jul 26 14:58:50 :: Resetting #131: Quagmire I (rooms 13100-13199).
Jul 26 14:58:50 :: Resetting #132: Quagmire II (rooms 13200-13299).
Jul 26 14:58:50 :: Resetting #133: Quagmire III (rooms 13300-13399).
Jul 26 14:58:50 :: Resetting #134: Mad Scientist Lab (rooms 13400-13499).
Jul 26 14:58:50 :: Resetting #135: Dudris's Unnamed Zone (rooms 13500-13599).
Jul 26 14:58:50 :: Resetting #136: Unnamed (rooms 13600-13699).
Jul 26 14:58:50 :: Resetting #137: Plains of Elysium (rooms 13700-13799).
Jul 26 14:58:50 :: Resetting #138: Out Riders Fortress (rooms 13800-13899).
Jul 26 14:58:50 :: Resetting #139: Unnamed (rooms 13900-13999).
Jul 26 14:58:50 :: Resetting #140: Wyvern City (rooms 14000-14099).
Jul 26 14:58:50 :: Resetting #141: Training Halls (rooms 14100-14599).
Jul 26 14:58:50 :: Resetting #143: reserved for training hall expansion (rooms 14300-14399).
Jul 26 14:58:50 :: Resetting #145: reserved for training hall expansion (rooms 14500-14599).
Jul 26 14:58:50 :: Resetting #147: Jithor's Unnamed Zone (rooms 14700-14799).
Jul 26 14:58:50 :: Resetting #148: Jharkendar walley (rooms 14800-14899).
Jul 26 14:58:50 :: Resetting #150: King Welmar's Castle (rooms 15000-15099).
Jul 26 14:58:50 :: Resetting #152: Azagh New Zone (rooms 15200-15299).
Jul 26 14:58:50 :: Resetting #154: Eastern Village (rooms 15400-15499).
Jul 26 14:58:50 :: Resetting #156: Cade's Unfinished Zone (rooms 15600-15699).
Jul 26 14:58:50 :: Resetting #158: Temple of Solomon (rooms 15800-15899).
Jul 26 14:58:50 :: Resetting #160: New Zone (rooms 16000-16099).
Jul 26 14:58:50 :: Resetting #162: New Zone (rooms 16200-16299).
Jul 26 14:58:50 :: Resetting #165: Tyrion's Revenge (rooms 16500-16599).
Jul 26 14:58:50 :: Resetting #167: The Silver Lining Inn (rooms 16700-16799).
Jul 26 14:58:50 :: Resetting #169: Gibberling Caves (rooms 16900-16999).
Jul 26 14:58:50 :: Resetting #172: New Zone - Darvin (rooms 17200-17299).
Jul 26 14:58:50 :: Resetting #175: Cardinal Wizards (rooms 17500-17599).
Jul 26 14:58:50 :: Resetting #177: New UnNamed Zone (rooms 17700-17799).
Jul 26 14:58:50 :: Resetting #180: Ornir Test Zone (rooms 18000-18099).
Jul 26 14:58:50 :: Resetting #183: New Zone (rooms 18300-18399).
Jul 26 14:58:50 :: Resetting #186: Newbie Zone (rooms 18600-18699).
Jul 26 14:58:50 :: Resetting #187: Circus (rooms 18700-18799).
Jul 26 14:58:50 :: Resetting #188: New Zone (rooms 18800-18899).
Jul 26 14:58:50 :: Resetting #190: The Depths: Living Forest (rooms 19000-19999).
Jul 26 14:58:50 :: Resetting #192: Training Halls (rooms 19200-19399).
Jul 26 14:58:50 :: Resetting #200: Western Highway (rooms 20000-20099).
Jul 26 14:58:50 :: Resetting #201: Sapphire Islands (rooms 20100-20199).
Jul 26 14:58:50 :: Resetting #203: Fernbrook Farm (rooms 20300-20399).
Jul 26 14:58:50 :: Resetting #204: Blackstone Keep (rooms 20400-20599).
Jul 26 14:58:50 :: Resetting #206: Coral Cove: Open Seas (rooms 20600-20699).
Jul 26 14:58:50 :: Resetting #207: Coral Cove: Coral Reef (rooms 20700-20899).
Jul 26 14:58:50 :: Resetting #210: The Onyx Obelisk (rooms 21000-21399).
Jul 26 14:58:50 :: Resetting #216: Training Halls (rooms 21600-21899).
Jul 26 14:58:50 :: Resetting #220: (Enchanted Kitchen) (rooms 22000-22099).
Jul 26 14:58:50 :: Resetting #222: The Grumblecakery (rooms 22200-22299).
Jul 26 14:58:50 :: Resetting #224: New Zone (rooms 22400-22499).
Jul 26 14:58:50 :: Resetting #232: Terringham (rooms 23200-23299).
Jul 26 14:58:50 :: Resetting #233: Dragon Plains (rooms 23300-23399).
Jul 26 14:58:50 :: Resetting #234: Newbie School (rooms 23400-23499).
Jul 26 14:58:50 :: Resetting #235: Dwarven Mines (rooms 23500-23599).
Jul 26 14:58:50 :: Resetting #236: Aldin (rooms 23600-23699).
Jul 26 14:58:50 :: Resetting #237: Dwarven Trade Route (rooms 23700-23799).
Jul 26 14:58:50 :: Resetting #238: Crystal Castle (rooms 23800-23899).
Jul 26 14:58:50 :: Resetting #239: South Pass (rooms 23900-23999).
Jul 26 14:58:50 :: Resetting #240: Dun Maura (rooms 24000-24099).
Jul 26 14:58:51 :: Resetting #241: (Starship Enterprise) (rooms 24100-24199).
Jul 26 14:58:51 :: Resetting #242: New Southern Midgen (rooms 24200-24299).
Jul 26 14:58:51 :: Resetting #243: Snowy Valley (rooms 24300-24399).
Jul 26 14:58:51 :: Resetting #244: Cooland Prison (rooms 24400-24499).
Jul 26 14:58:51 :: Resetting #245: The Nether (rooms 24500-24599).
Jul 26 14:58:51 :: Resetting #246: The Nether II (rooms 24600-24699).
Jul 26 14:58:51 :: Resetting #247: Graveyard (rooms 24700-24799).
Jul 26 14:58:51 :: Resetting #248: Elven Woods (rooms 24800-24899).
Jul 26 14:58:51 :: Resetting #249: Untitled (Jedi Clan House) (rooms 24900-24999).
Jul 26 14:58:51 :: Resetting #250: DragonSpyre (rooms 25000-25099).
Jul 26 14:58:51 :: Resetting #251: Ape Village (rooms 25100-25199).
Jul 26 14:58:51 :: Resetting #252: Castle of the Vampyre (rooms 25200-25299).
Jul 26 14:58:51 :: Resetting #253: Windmill (rooms 25300-25399).
Jul 26 14:58:51 :: Resetting #254: Mordecai's Village (rooms 25400-25499).
Jul 26 14:58:51 :: Resetting #255: Shipwreck (rooms 25500-25599).
Jul 26 14:58:51 :: Resetting #256: Lord's Keep (rooms 25600-25699).
Jul 26 14:58:51 :: Resetting #257: Jareth Main City (rooms 25700-25799).
Jul 26 14:58:51 :: Resetting #258: Light Forest (rooms 25800-25899).
Jul 26 14:58:51 :: Resetting #259: Haunted Mansion (rooms 25900-25999).
Jul 26 14:58:51 :: Resetting #260: Grasslands (rooms 26000-26099).
Jul 26 14:58:51 :: Resetting #261: Inna & Igor's Castle (rooms 26100-26199).
Jul 26 14:58:51 :: Resetting #262: Forest Trails (rooms 26200-26299).
Jul 26 14:58:51 :: Resetting #263: Farmlands (rooms 26300-26399).
Jul 26 14:58:51 :: Resetting #264: Banshide (rooms 26400-26499).
Jul 26 14:58:51 :: Resetting #265: Beach & Lighthouse (rooms 26500-26599).
Jul 26 14:58:51 :: Resetting #266: Realm of Lord Ankou (rooms 26600-26699).
Jul 26 14:58:51 :: Resetting #267: Vice Island (rooms 26700-26799).
Jul 26 14:58:51 :: Resetting #268: Vice Island II (rooms 26800-26899).
Jul 26 14:58:51 :: Resetting #269: Southern Desert (rooms 26900-26999).
Jul 26 14:58:51 :: Resetting #270: Wasteland (rooms 27000-27099).
Jul 26 14:58:52 :: Resetting #271: Sundhaven (rooms 27100-27199).
Jul 26 14:58:52 :: Resetting #272: Sundhaven II (rooms 27200-27299).
Jul 26 14:58:52 :: Resetting #273: (Space Station Alpha) (rooms 27300-27399).
Jul 26 14:58:52 :: Resetting #274: Portablo (Smurfville) (rooms 27400-27499).
Jul 26 14:58:52 :: Resetting #275: New Sparta (rooms 27500-27599).
Jul 26 14:58:52 :: Resetting #276: New Sparta II (rooms 27600-27699).
Jul 26 14:58:52 :: Resetting #277: Shire (rooms 27700-27799).
Jul 26 14:58:52 :: Resetting #278: Oceania (rooms 27800-27899).
Jul 26 14:58:52 :: Resetting #279: Notre Dame (rooms 27900-27999).
Jul 26 14:58:52 :: Resetting #280: (Living Motherboard) (rooms 28000-28099).
Jul 26 14:58:52 :: Resetting #281: Forest of Khanjar (rooms 28100-28199).
Jul 26 14:58:52 :: Resetting #282: Infernal (rooms 28200-28299).
Jul 26 14:58:52 :: Resetting #283: Haunted House (rooms 28300-28399).
Jul 26 14:58:52 :: Resetting #284: Ghenna (rooms 28400-28499).
Jul 26 14:58:52 :: Resetting #285: Descent to Hell II (rooms 28500-28599).
Jul 26 14:58:52 :: Resetting #286: Descent to Hell (rooms 28600-28699).
Jul 26 14:58:52 :: Resetting #287: Ofingia / Goblin Town (rooms 28700-28799).
Jul 26 14:58:53 :: Resetting #288: Galaxy (rooms 28800-28899).
Jul 26 14:58:53 :: Resetting #289: Werith's Wayhouse (rooms 28900-28999).
Jul 26 14:58:53 :: Resetting #290: Lizard Lair (rooms 29000-29099).
Jul 26 14:58:53 :: Resetting #291: Black Forest (rooms 29100-29199).
Jul 26 14:58:53 :: Resetting #292: Kerofk (rooms 29200-29299).
Jul 26 14:58:53 :: Resetting #293: Kerofk II (rooms 29300-29399).
Jul 26 14:58:53 :: Resetting #294: Trade Road (rooms 29400-29499).
Jul 26 14:58:53 :: Resetting #295: Jungle (rooms 29500-29599).
Jul 26 14:58:53 :: Resetting #296: Froboz Fun Factory (rooms 29600-29699).
Jul 26 14:58:53 :: Resetting #298: Castle of Desire (rooms 29800-29899).
Jul 26 14:58:53 :: Resetting #299: Abandoned Cathedral (rooms 29900-29999).
Jul 26 14:58:53 :: Resetting #300: Ancalador (rooms 30000-30099).
Jul 26 14:58:53 :: Resetting #301: Campus (rooms 30100-30199).
Jul 26 14:58:53 :: Resetting #302: Campus II (rooms 30200-30299).
Jul 26 14:58:53 :: Resetting #303: Campus III (rooms 30300-30399).
Jul 26 14:58:53 :: Resetting #304: Temple of the Bull (rooms 30400-30499).
Jul 26 14:58:53 :: Resetting #305: Chessboard (rooms 30500-30599).
Jul 26 14:58:53 :: Resetting #306: Newbie Tree (rooms 30600-30699).
Jul 26 14:58:53 :: Resetting #307: Castle (rooms 30700-30799).
Jul 26 14:58:53 :: Resetting #308: Baron Cailveh (rooms 30800-30899).
Jul 26 14:58:53 :: Resetting #309: Keep of Baron Westlawn (rooms 30900-30999).
Jul 26 14:58:53 :: Resetting #310: Graye Area (rooms 31000-31099).
Jul 26 14:58:53 :: Resetting #311: The Dragon's Teeth (rooms 31100-31199).
Jul 26 14:58:53 :: Resetting #312: Leper Island (rooms 31200-31299).
Jul 26 14:58:53 :: Resetting #313: Farmlands of Ofingia (rooms 31300-31399).
Jul 26 14:58:54 :: Resetting #314: X'Raantra's Altar (rooms 31400-31499).
Jul 26 14:58:54 :: Resetting #315: McGintey Business District (rooms 31500-31599).
Jul 26 14:58:54 :: Resetting #316: McGintey Guild Area (rooms 31600-31699).
Jul 26 14:58:54 :: Resetting #317: Wharf (rooms 31700-31799).
Jul 26 14:58:54 :: Resetting #318: Dock Area (rooms 31800-31899).
Jul 26 14:58:54 :: Resetting #319: Yllythad Sea (rooms 31900-31999).
Jul 26 14:58:54 :: Resetting #320: Yllythad Sea II (rooms 32000-32099).
Jul 26 14:58:54 :: Resetting #321: Yllythad Sea III (rooms 32100-32199).
Jul 26 14:58:54 :: Resetting #322: McGintey Bay (rooms 32200-32299).
Jul 26 14:58:54 :: Resetting #323: Caverns of the Pale Man (rooms 32300-32399).
Jul 26 14:58:54 :: Resetting #324: Army Encampment (rooms 32400-32499).
Jul 26 14:58:54 :: Resetting #325: Revelry (rooms 32500-32599).
Jul 26 14:58:54 :: Resetting #326: Army Perimeter (rooms 32600-32699).
Jul 26 14:58:54 :: Resetting #328: Anvil's Boreal Forest (rooms 32800-32899).
Jul 26 14:58:54 :: Resetting #330: Dwarven Royal Chambers (rooms 33000-33099).
Jul 26 14:58:54 :: Resetting #345: Fire Giant Keep (rooms 34500-34699).
Jul 26 14:58:54 :: Resetting #347: A Ruined Farm (VQL) (rooms 34700-34799).
Jul 26 14:58:54 :: Resetting #348: New Zone (rooms 34800-34899).
Jul 26 14:58:54 :: Resetting #400: Astral Plane (rooms 40000-40099).
Jul 26 14:58:54 :: Resetting #401: Ethereal Plane (rooms 40100-40199).
Jul 26 14:58:54 :: Resetting #402: Elemental Plane (rooms 40200-40299).
Jul 26 14:58:54 :: Resetting #403: Mystery Warehouse (rooms 40300-40399).
Jul 26 14:58:54 :: Resetting #404: Blindbreak Rest (rooms 40400-40499).
Jul 26 14:58:54 :: Resetting #405: the Pirate Ship "Bloody Rum" (rooms 40500-40599).
Jul 26 14:58:54 :: Resetting #406: Mosaic Cave (rooms 40600-40699).
Jul 26 14:58:55 :: Resetting #407: Dark Darkling Crypt (rooms 40700-40799).
Jul 26 14:58:55 :: Resetting #408: Kayos Test Zone For Toril (rooms 40800-40899).
Jul 26 14:58:55 :: Resetting #555: Ultima (rooms 55500-55599).
Jul 26 14:58:55 :: Resetting #556: Ultima II (rooms 55600-55699).
Jul 26 14:58:55 :: Resetting #600: <*> Monster Manual [RESERVED Zones 600-623] (rooms 60000-62299).
Jul 26 14:58:55 :: Resetting #655: <*> Internal NOWHERE (rooms 65500-65534).
Jul 26 14:58:55 :: Resetting #666: <*> Internal NOWHERE (rooms 66600-66699).
Jul 26 14:58:55 :: Resetting #667: <*> RESERVED Transport Rooms (rooms 66700-66799).
Jul 26 14:58:55 :: Resetting #1001: <*> Internal Common Items (rooms 100100-100499).
Jul 26 14:58:55 :: Resetting #1005: <*> Random Encounter Distro (rooms 100500-100999).
Jul 26 14:58:55 :: Resetting #1010: <*> Quest Zone (rooms 101000-101199).
Jul 26 14:58:55 :: Resetting #1012: <*> Extended Staff Area (rooms 101200-101399).
Jul 26 14:58:55 :: Resetting #1014: <*> Spirit Sanctuary (rooms 101400-101499).
Jul 26 14:58:55 :: Resetting #1015: The Trade Way (rooms 101500-101699).
Jul 26 14:58:55 :: Resetting #1017: The Ruined Keep (rooms 101700-101799).
Jul 26 14:58:55 :: Resetting #1018: Dawn Pass & Lonely Moor (rooms 101800-101999).
Jul 26 14:58:55 :: Resetting #1020: Delimiyr Route (rooms 102000-102099).
Jul 26 14:58:55 :: Resetting #1021: The High Road - North (rooms 102100-102299).
Jul 26 14:58:55 :: Resetting #1023: The Long Road (rooms 102300-102399).
Jul 26 14:58:55 :: Resetting #1024: Skull Gorge (rooms 102400-102499).
Jul 26 14:58:55 :: Resetting #1025: Bloodfist Caverns (rooms 102500-102799).
Jul 26 14:58:55 :: Resetting #1030: Ashenport (rooms 103000-103899).
Jul 26 14:58:55 :: Resetting #1039: North of Ashenport (rooms 103900-103999).
Jul 26 14:58:55 :: Resetting #1040: The Coast Way (rooms 104000-104299).
Jul 26 14:58:55 :: Resetting #1043: <*> Mercenary Camps (rooms 104300-104399).
Jul 26 14:58:55 :: Resetting #1044: The Stag Forest (rooms 104400-104499).
Jul 26 14:58:55 :: Resetting #1045: Eastern Roads (rooms 104500-104699).
Jul 26 14:58:55 :: Resetting #1047: Bargewright Inn (rooms 104700-104899).
Jul 26 14:58:55 :: Resetting #1049: Amphail (rooms 104900-104999).
Jul 26 14:58:55 :: Resetting #1050: Corm Orp (rooms 105000-105199).
Jul 26 14:58:56 :: Resetting #1052: Corm Orp Caverns (rooms 105200-105299).
Jul 26 14:58:56 :: Resetting #1053: The Way Inn (rooms 105300-105399).
Jul 26 14:58:56 :: Resetting #1054: Eveningstar (rooms 105400-105599).
Jul 26 14:58:56 :: Resetting #1056: Gracklstugh (rooms 105600-105899).
Jul 26 14:58:56 :: Resetting #1060: Crimson Flame (rooms 106000-106199).
Jul 26 14:58:56 :: Resetting #1062: Orc Ruins (rooms 106200-106399).
Jul 26 14:58:56 :: Resetting #1064: Elven Settlement (rooms 106400-106599).
Jul 26 14:58:56 :: Resetting #1066: Red Larch (rooms 106600-106699).
Jul 26 14:58:56 :: Resetting #1067: Fire Giants (rooms 106700-106799).
Jul 26 14:58:56 :: Resetting #1068: Longsaddle (rooms 106800-106899).
Jul 26 14:58:56 :: Resetting #1070: A Dwarven Stronghold (rooms 107000-107099).
Jul 26 14:58:56 :: Resetting #1071: Gentle Oak Valley (rooms 107100-107199).
Jul 26 14:58:56 :: Resetting #1072: The Trade Way II (rooms 107200-107399).
Jul 26 14:58:56 :: Resetting #1075: Shadowdale (rooms 107500-107799).
Jul 26 14:58:56 :: Resetting #1078: Aumvor's Castle (rooms 107800-107999).
Jul 26 14:58:57 :: Resetting #1081: Mithril Hall (rooms 108100-108599).
Jul 26 14:58:57 :: Resetting #1086: The Dusk Road (rooms 108600-108699).
Jul 26 14:58:57 :: Resetting #1087: The High Road South (rooms 108700-108799).
Jul 26 14:58:57 :: Resetting #1088: The Moonsea Ride (rooms 108800-108899).
Jul 26 14:58:57 :: Resetting #1089: The North Ride (rooms 108900-108999).
Jul 26 14:58:57 :: Resetting #1090: The Evermoor Way (rooms 109000-109099).
Jul 26 14:58:57 :: Resetting #1091: The Rauvin Ride (rooms 109100-109199).
Jul 26 14:58:57 :: Resetting #1092: Ogre Lair (rooms 109200-109299).
Jul 26 14:58:57 :: Resetting #1094: Tesh Trail (rooms 109400-109499).
Jul 26 14:58:57 :: Resetting #1095: Dagger Falls (rooms 109500-109699).
Jul 26 14:58:57 :: Resetting #1097: The Neverwinter Wood (rooms 109700-109999).
Jul 26 14:58:57 :: Resetting #1100: The Tower of Twilight (rooms 110000-110099).
Jul 26 14:58:57 :: Resetting #1101: Ardeep Forest (rooms 110100-110299).
Jul 26 14:58:57 :: Resetting #1103: The Stump Bog (rooms 110300-110399).
Jul 26 14:58:57 :: Resetting #1104: Tugrahk Gol (rooms 110400-110499).
Jul 26 14:58:57 :: Resetting #1106: Lost City of Thunderholme (rooms 110600-110699).
Jul 26 14:58:57 :: Resetting #1107: Westwood (rooms 110700-110799).
Jul 26 14:58:57 :: Resetting #1108: Skull Crag (rooms 110800-110999).
Jul 26 14:58:57 :: Resetting #1110: Tethyamar Trail (rooms 111000-111099).
Jul 26 14:58:57 :: Resetting #1113: Tilverton (rooms 111300-111499).
Jul 26 14:58:57 :: Resetting #1115: Bleak Palace (rooms 111500-111699).
Jul 26 14:58:57 :: Resetting #1117: <*> Spirit Walk (rooms 111700-111899).
Jul 26 14:58:57 :: Resetting #1119: <*> Spirit Walk (rooms 111900-112099).
Jul 26 14:58:57 :: Resetting #1121: Plane of the Abyss (rooms 112100-112299).
Jul 26 14:58:57 :: Resetting #1123: Mount Hotenow (rooms 112300-112399).
Jul 26 14:58:57 :: Resetting #1124: The Fire Plane (rooms 112400-112599).
Jul 26 14:58:57 :: Resetting #1126: Flaming Tower (rooms 112600-112999).
Jul 26 14:58:57 :: Resetting #1130: Mantol-Derith (rooms 113000-113199).
Jul 26 14:58:57 :: Resetting #1132: Mantol-Derith Tunnels (rooms 113200-113299).
Jul 26 14:58:57 :: Resetting #1133: Ancient Mines (rooms 113300-113399).
Jul 26 14:58:58 :: Resetting #1134: Candlekeep Proper (rooms 113400-113699).
Jul 26 14:58:58 :: Resetting #1137: Ashabenford (rooms 113700-113899).
Jul 26 14:58:58 :: Resetting #1141: Soubar Underhalls (rooms 114100-114199).
Jul 26 14:58:58 :: Resetting #1142: Rogue's Lair (rooms 114200-114399).
Jul 26 14:58:58 :: Resetting #1144: Temple in the Sky (rooms 114400-114699).
Jul 26 14:58:58 :: Resetting #1147: Kobold Caverns (rooms 114700-114799).
Jul 26 14:58:58 :: Resetting #1148: South Wood (rooms 114800-114999).
Jul 26 14:58:58 :: Resetting #1150: Cloud Labyrinth (rooms 115000-115299).
Jul 26 14:58:58 :: Resetting #1153: The Friendly Arm (rooms 115300-115399).
Jul 26 14:58:58 :: Resetting #1155: The Rat Hills (rooms 115500-115699).
Jul 26 14:58:58 :: Resetting #1157: Hidden Mine (rooms 115700-115899).
Jul 26 14:58:58 :: Resetting #1159: The Abbey of Lost Tears (rooms 115900-115999).
Jul 26 14:58:58 :: Resetting #1160: The Abyss (rooms 116000-116199).
Jul 26 14:58:58 :: Resetting #1170: Dragonspear Castle (rooms 117000-117399).
Jul 26 14:58:58 :: Resetting #1174: Grunwald (rooms 117400-117699).
Jul 26 14:58:58 :: Resetting #1177: Eveningstar Haunted Halls (rooms 117700-117899).
Jul 26 14:58:58 :: Resetting #1179: Triel (rooms 117900-117999).
Jul 26 14:58:59 :: Resetting #1180: Logger's Camp (rooms 118000-118099).
Jul 26 14:58:59 :: Resetting #1181: Caverns of the Underdark (rooms 118100-118299).
Jul 26 14:58:59 :: Resetting #1183: Serpent Path (rooms 118300-118399).
Jul 26 14:58:59 :: Resetting #1184: Crystal Caverns (rooms 118400-118499).
Jul 26 14:58:59 :: Resetting #1185: Hardbuckler (rooms 118500-118699).
Jul 26 14:58:59 :: Resetting #1187: The Lost Vale (rooms 118700-118899).
Jul 26 14:58:59 :: Resetting #1189: Yellow Snake Pass (rooms 118900-119099).
Jul 26 14:58:59 :: Resetting #1191: Llawryn Keep (rooms 119100-119599).
Jul 26 14:58:59 :: Resetting #1196: Sunken Pirate Ship (rooms 119600-119699).
Jul 26 14:58:59 :: Resetting #1197: Sewers of CandleKeep (rooms 119700-119899).
Jul 26 14:58:59 :: Resetting #1199: Hardbuckler Caverns (rooms 119900-119999).
Jul 26 14:58:59 :: Resetting #1200: Zhent Graveyard (rooms 120000-120099).
Jul 26 14:58:59 :: Resetting #1201: Bugbear Caverns (rooms 120100-120199).
Jul 26 14:58:59 :: Resetting #1202: Barbarian Encampment (rooms 120200-120299).
Jul 26 14:58:59 :: Resetting #1203: Kuo-Toa Village (rooms 120300-120399).
Jul 26 14:58:59 :: Resetting #1204: Hill's Edge I (rooms 120400-120799).
Jul 26 14:58:59 :: Resetting #1208: Evereska Way (rooms 120800-120899).
Jul 26 14:58:59 :: Resetting #1209: City of Mirabar (rooms 120900-121099).
Jul 26 14:58:59 :: Resetting #1211: Dark Dominion (rooms 121100-121199).
Jul 26 14:58:59 :: Resetting #1212: Lizard Marsh (rooms 121200-121399).
Jul 26 14:58:59 :: Resetting #1214: Arabel (rooms 121400-121649).
Jul 26 14:58:59 :: Resetting #1217: Soubar (rooms 121700-121799).
Jul 26 14:58:59 :: Resetting #1218: Beregost (rooms 121800-121899).
Jul 26 14:59:00 :: Resetting #1219: Underdark Tunnels (rooms 121900-121999).
Jul 26 14:59:00 :: Resetting #1220: <*> Trade Center Zone (rooms 122000-122099).
Jul 26 14:59:00 :: Resetting #1221: Empty Zone (rooms 122100-122199).
Jul 26 14:59:00 :: Resetting #1222: Luskan Outpost (rooms 122200-122299).
Jul 26 14:59:00 :: Resetting #1224: Neverwinter - North City (rooms 122400-122599).
Jul 26 14:59:00 :: Resetting #1226: Neverwinter - South City (rooms 122600-122899).
Jul 26 14:59:00 :: Resetting #1229: Neverwinter - Cloaktower (rooms 122900-122999).
Jul 26 14:59:00 :: Resetting #1230: Neverwinter - Castle (rooms 123000-123199).
Jul 26 14:59:00 :: Resetting #1232: Neverwinter - Catacombs (rooms 123200-123399).
Jul 26 14:59:00 :: Resetting #1234: Neverwinter - Sewers (rooms 123400-123699).
Jul 26 14:59:00 :: Resetting #1237: Neverwinter - Upland Rise (rooms 123700-123799).
Jul 26 14:59:00 :: Resetting #1250: Secomber (rooms 125000-125299).
Jul 26 14:59:01 :: Resetting #1253: Iron Road (rooms 125300-125499).
Jul 26 14:59:01 :: Resetting #1255: Darklake (rooms 125500-125899).
Jul 26 14:59:01 :: Resetting #1259: Pesh (rooms 125900-125999).
Jul 26 14:59:01 :: Resetting #1260: High Horn (rooms 126000-126199).
Jul 26 14:59:01 :: Resetting #1262: Misty Forest (rooms 126200-126299).
Jul 26 14:59:01 :: Resetting #1263: Mithril Hall Palace (rooms 126300-126399).
Jul 26 14:59:01 :: Resetting #1264: Roads of Amn (rooms 126400-126599).
Jul 26 14:59:01 :: Resetting #1266: Trollclaws (rooms 126600-126699).
Jul 26 14:59:01 :: Resetting #1267: Mere of Dead Men (rooms 126700-126899).
Jul 26 14:59:01 :: Resetting #1269: Illithid Enclave (rooms 126900-126999).
Jul 26 14:59:01 :: Resetting #1272: The Reaching Woods (rooms 127200-127399).
Jul 26 14:59:01 :: Resetting #1274: Goblinoid Warrens (rooms 127400-127499).
Jul 26 14:59:01 :: Resetting #1275: Evereska (rooms 127500-127999).
Jul 26 14:59:01 :: Resetting #1280: The Halfway Inn (rooms 128000-128099).
Jul 26 14:59:01 :: Resetting #1281: The Labyrinth (rooms 128100-128599).
Jul 26 14:59:01 :: Resetting #1286: Greycloak Hills Side Path (rooms 128600-128999).
Jul 26 14:59:01 :: Resetting #1290: The Astral Plane (rooms 129000-129199).
Jul 26 14:59:02 :: Resetting #1292: The Lurkwood (rooms 129200-129399).
Jul 26 14:59:02 :: Resetting #1294: King's Lodge (rooms 129400-129499).
Jul 26 14:59:02 :: Resetting #1295: The Ethereal Plane (rooms 129500-129699).
Jul 26 14:59:02 :: Resetting #1297: Forest of Wyrms (rooms 129700-129899).
Jul 26 14:59:02 :: Resetting #1299: Crypts of Darekoth (rooms 129900-129999).
Jul 26 14:59:02 :: Resetting #1300: Undermountain [Level I] (rooms 130000-130599).
Jul 26 14:59:02 :: Resetting #1321: Avernus (rooms 132100-132399).
Jul 26 14:59:02 :: Resetting #1325: The Serpent Hills (rooms 132500-132699).
Jul 26 14:59:02 :: Resetting #1327: Snake Pit (rooms 132700-132799).
Jul 26 14:59:02 :: Resetting #1328: Amiskal's Keep (rooms 132800-132899).
Jul 26 14:59:02 :: Resetting #1329: Tower of Kenjin (rooms 132900-132999).
Jul 26 14:59:02 :: Resetting #1330: Settlestone (rooms 133000-133099).
Jul 26 14:59:02 :: Resetting #1331: The Plane of Shadows (rooms 133100-133199).
Jul 26 14:59:02 :: Resetting #1332: Ulcaster's College (rooms 133200-133299).
Jul 26 14:59:02 :: Resetting #1335: Wormwrithings (rooms 133500-133699).
Jul 26 14:59:02 :: Resetting #1337: The East Way (rooms 133700-133899).
Jul 26 14:59:02 :: Resetting #1339: High Forest (South) (rooms 133900-134099).
Jul 26 14:59:02 :: Resetting #1341: High Forest II (rooms 134100-134299).
Jul 26 14:59:02 :: Resetting #1350: Menzoberranzan (rooms 135000-135999).
Jul 26 14:59:02 :: Resetting #1360: Deep Eveningstar Halls (rooms 136000-136099).
Jul 26 14:59:02 :: Resetting #1361: Air Plane (rooms 136100-136299).
Jul 26 14:59:02 :: Resetting #1363: Water Plane (rooms 136300-136499).
Jul 26 14:59:02 :: Resetting #1365: Temple of Ghaundaur (rooms 136500-136699).
Jul 26 14:59:02 :: Resetting #1367: Earth Plane (rooms 136700-136899).
Jul 26 14:59:02 :: Resetting #1369: The Deep Caverns (rooms 136900-136999).
Jul 26 14:59:02 :: Resetting #1370: Battle of Bones (rooms 137000-137199).
Jul 26 14:59:02 :: Resetting #1372: Ch'Chitl (rooms 137200-137499).
Jul 26 14:59:02 :: Resetting #1375: Malaugrym Castle (rooms 137500-137799).
Jul 26 14:59:02 :: Resetting #1380: The Stonelands (rooms 138000-138399).
Jul 26 14:59:03 :: Resetting #1384: Daurgothoth's Domain (rooms 138400-138499).
Jul 26 14:59:03 :: Resetting #1385: Dwarven Mines (rooms 138500-138599).
Jul 26 14:59:03 :: Resetting #1386: <*> Arena (rooms 138600-138699).
Jul 26 14:59:03 :: Resetting #1387: Dragon Cult Fortress (rooms 138700-138799).
Jul 26 14:59:03 :: Resetting #1388: Zzsessak Zuhl (rooms 138800-139099).
Jul 26 14:59:03 :: Resetting #1391: The Grimlock Burrows (rooms 139100-139199).
Jul 26 14:59:03 :: Resetting #1392: Abyssal Vortex (rooms 139200-139299).
Jul 26 14:59:03 :: Resetting #1393: The Hive of Passion (rooms 139300-139399).
Jul 26 14:59:03 :: Resetting #1394: Northeastern Underdark (rooms 139400-139799).
Jul 26 14:59:03 :: Resetting #1398: The Plane of Magma (rooms 139800-139999).
Jul 26 14:59:03 :: Resetting #1402: Scorched Forest (rooms 140200-140299).
Jul 26 14:59:03 :: Resetting #1403: Urd Caverns (rooms 140300-140399).
Jul 26 14:59:03 :: Resetting #1408: Outcast Encampment (rooms 140800-140899).
Jul 26 14:59:03 :: Resetting #1409: Mines of Tugrahk Gol (rooms 140900-140999).
Jul 26 14:59:03 :: Resetting #1416: Ashenport Harbor (rooms 141600-141799).
Jul 26 14:59:03 :: Resetting #1418: Serene Forest (rooms 141800-141899).
Jul 26 14:59:03 :: Resetting #1419: Amenth'G'narr (rooms 141900-141999).
Jul 26 14:59:03 :: Resetting #1420: Elg'cahl Niar (rooms 142000-142099).
Jul 26 14:59:03 :: Resetting #1423: The Abyssal Stronghold (rooms 142300-142499).
Jul 26 14:59:03 :: Resetting #1425: Daggerford (rooms 142500-142799).
Jul 26 14:59:03 :: Resetting #1430: High Forest - East (rooms 143000-143199).
Jul 26 14:59:03 :: Resetting #1432: Beneath the Eagletower (rooms 143200-143299).
Jul 26 14:59:03 :: Resetting #1433: Thethyr Bandit Castle (rooms 143300-143399).
Jul 26 14:59:03 :: Resetting #1434: Bee Zone (rooms 143400-143499).
Jul 26 14:59:03 :: Resetting #1436: Plane of Ice (rooms 143600-143999).
Jul 26 14:59:03 :: Resetting #1440: Ardeep Forest (rooms 144000-144199).
Jul 26 14:59:03 :: Resetting #1445: Cloud Realm of Stronmaus (rooms 144500-144999).
Jul 26 14:59:03 :: Resetting #1451: Temple of Twisted Flesh (rooms 145100-145199).
Jul 26 14:59:04 :: Resetting #1452: Mosswood (rooms 145200-145399).
Jul 26 14:59:04 :: Resetting #1455: Sloopdilmonpolop (rooms 145500-146199).
Jul 26 14:59:04 :: Resetting #1468: Trollbark Forest (rooms 146800-146999).
Jul 26 14:59:04 :: Resetting #1480: Arath Zuul (rooms 148000-148099).
Jul 26 14:59:04 :: Resetting #1481: Orcish Fort (rooms 148100-148199).
Jul 26 14:59:04 :: SYSERR: zone file: invalid equipment pos number (mob 	RHigh Priest	D of Grummsh	n, obj 109, pos 148181)
Jul 26 14:59:04 :: SYSERR: ...offending cmd: 'E' cmd in zone #1481, line 31
Jul 26 14:59:04 :: Resetting #1500: <*> Quest Staging Area (rooms 150000-150599).
Jul 26 14:59:04 :: Resetting #1507: Hulburg Trail (rooms 150700-150899).
Jul 26 14:59:04 :: Resetting #1555: Labyrinth of the Mad Drow (rooms 155500-155699).
Jul 26 14:59:04 :: Resetting #1557: <*> Random Encounter Distro (rooms 155700-156199).
Jul 26 14:59:04 :: Resetting #1575: Luskan Southbank (rooms 157500-157999).
Jul 26 14:59:04 :: Resetting #1591: Hulburg (rooms 159100-159599).
Jul 26 14:59:05 :: Resetting #1596: Minotaur Outpost (rooms 159600-159699).
Jul 26 14:59:05 :: Resetting #1688: Evermeet Main Rd (rooms 168800-168899).
Jul 26 14:59:05 :: Resetting #1689: Evermeet E Coast Rd N (rooms 168900-168999).
Jul 26 14:59:05 :: Resetting #1690: Evermeet W Coast Rd N (rooms 169000-169099).
Jul 26 14:59:05 :: Resetting #1691: Evermeet Ancient Forest (rooms 169100-169199).
Jul 26 14:59:05 :: Resetting #1692: Evermeet Misc Rooms/Mobs (rooms 169200-169299).
Jul 26 14:59:05 :: Resetting #1693: Evermeet Rd to Elven Settl (rooms 169300-169399).
Jul 26 14:59:05 :: Resetting #1800: MarblePyramid HighForest E (rooms 180000-180299).
Jul 26 14:59:05 :: Resetting #1803: MarblePyramid HighForest I (rooms 180300-180499).
Jul 26 14:59:05 :: Resetting #1960: Jotunheim (rooms 196000-196299).
Jul 26 14:59:05 :: Resetting #2000: <*> Guildhalls (rooms 200000-200099).
Jul 26 14:59:05 :: Resetting #2001: Treshia (rooms 200100-200199).
Jul 26 14:59:05 :: Resetting #2002: Treshia Forest (rooms 200200-200299).
Jul 26 14:59:05 :: Resetting #2003: Within Mosswood Forest (rooms 200300-200399).
Jul 26 14:59:05 :: Resetting #5515: Delimbiyr Vale (rooms 551500-551999).
Jul 26 14:59:05 :: Resetting #5520: Road West of Secomber (rooms 552000-552099).
Jul 26 14:59:05 :: Resetting #5521: Road through Orlbar (rooms 552100-552299).
Jul 26 14:59:05 :: Resetting #5523: Skullport Port & Island (rooms 552300-552699).
Jul 26 14:59:05 :: Resetting #5527: Skullport Trade Lanes (rooms 552700-552899).
Jul 26 14:59:05 :: Resetting #5529: Skullport Heart (rooms 552900-553099).
Jul 26 14:59:05 :: Resetting #5531: Pelleor's Prairie (rooms 553100-553299).
Jul 26 14:59:05 :: Resetting #5536: Hellgate Keep (rooms 553600-554099).
Jul 26 14:59:05 :: Resetting #9999: New Zone (rooms 999900-999999).
Jul 26 14:59:05 :: Resetting #10000: <*> Wilderness of Luminari (rooms 1000000-1009999).
Jul 26 14:59:05 :: Resetting #12157521: <*> THE UPPER LIMIT (rooms 1215752100-1215752191).
Jul 26 14:59:05 :: Booting houses.
Jul 26 14:59:07 :: Boot db -- DONE.
Jul 26 14:59:07 :: Signal trapping.
Jul 26 14:59:07 :: Entering game loop.
Jul 26 14:59:07 :: No connections.  Going to sleep.
Jul 26 14:59:14 :: New connection.  Waking up.
Jul 26 14:59:14 :: Pulse usage new high water mark [0.68%, 682 usec]. Trace info: 
Pulse profiling info

Section name        | Enter Count|  Exit Count|  usec total|     pulse %|max pulse % (1 entry)
------------------------------------------------------------------------------- 
do_action           |           1|           1|        2070|       2.07%|               2.07%
do_follow           |          13|          13|        4986|       4.99%|               3.68%
do_gen_door         |           1|           1|        7858|       7.86%|               7.86%
do_grab             |           2|           2|        5429|       5.43%|               3.64%
do_group            |           2|           2|        2747|       2.75%|               1.58%
do_mload            |         121|         121|       31670|      31.67%|               3.35%
do_mpurge           |           1|           1|         767|       0.77%|               0.77%
do_mteleport        |           2|           2|        4889|       4.89%|               4.34%
do_put              |           1|           1|        1768|       1.77%|               1.77%
do_rescue           |           2|           2|         657|       0.66%|               0.65%
do_sit              |           3|           3|        1626|       1.63%|               0.82%
do_stand            |           2|           2|        1008|       1.01%|               0.96%
do_wear             |          71|          71|      124639|     124.64%|              12.31%
do_wield            |          20|          20|       26754|      26.75%|               3.69%
questmaster         |           5|           5|        1161|       1.16%|               1.11%

Jul 26 14:59:15 :: Pulse usage new high water mark [20.53%, 20530 usec]. Trace info: 
Pulse profiling info

Section name        | Enter Count|  Exit Count|  usec total|     pulse %|max pulse % (1 entry)
------------------------------------------------------------------------------- 
Main Loop           |           1|           1|       18491|      18.49%|              18.49%
Process Commands    |           1|           1|         308|       0.31%|               0.31%
Process Input       |           1|           1|         346|       0.35%|               0.35%
Process Output      |           1|           1|         395|       0.40%|               0.40%
do_gen_door         |           1|           1|         869|       0.87%|               0.87%
do_wear             |           1|           1|         181|       0.18%|               0.18%
event_process       |           1|           1|        6955|       6.96%|               6.96%
heartbeat           |           1|           1|        9103|       9.10%|               9.10%
shop_keeper         |           1|           1|         672|       0.67%|               0.67%

Jul 26 14:59:15 :: Pulse usage new high water mark [37.88%, 37879 usec]. Trace info: 
Pulse profiling info

Section name        | Enter Count|  Exit Count|  usec total|     pulse %|max pulse % (1 entry)
------------------------------------------------------------------------------- 
Main Loop           |           1|           1|       37673|      37.67%|              37.67%
Process Commands    |           1|           1|           2|       0.00%|               0.00%
Process Input       |           1|           1|        3408|       3.41%|               3.41%
Process Output      |           1|           1|         250|       0.25%|               0.25%
event_process       |           1|           1|       33984|      33.98%|              33.98%
heartbeat           |           1|           1|       33990|      33.99%|              33.99%

Jul 26 14:59:20 :: PERFMON: High load detected (171.2%), enabling full monitoring
Jul 26 14:59:20 :: Pulse usage new high water mark [171.22%, 171217 usec]. Trace info: 
Pulse profiling info

Section name        | Enter Count|  Exit Count|  usec total|     pulse %|max pulse % (1 entry)
------------------------------------------------------------------------------- 
Main Loop           |           1|           1|      170997|     171.00%|             171.00%
Process Commands    |           1|           1|           2|       0.00%|               0.00%
Process Input       |           1|           1|           2|       0.00%|               0.00%
Process Output      |           1|           1|           2|       0.00%|               0.00%
event_process       |           1|           1|           3|       0.00%|               0.00%
heartbeat           |           1|           1|      170963|     170.96%|             170.96%
msdp_update         |           1|           1|         272|       0.27%|               0.27%
pulse_luminari      |           1|           1|      169520|     169.52%|             169.52%

Jul 26 14:59:20 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 14:59:20 :: SYSERR: 	YBrother Spire	n (#200103): Attempting to call non-existing mob function.
Jul 26 14:59:21 :: SYSERR: Jakur the tanner (#125913): Attempting to call non-existing mob function.
Jul 26 14:59:21 :: Board 0 initialized: vnum=2201, rnum=772
Jul 26 14:59:21 :: SYSERR: Adoril (#21605): Attempting to call non-existing mob function.
Jul 26 14:59:21 :: PERFMON: High load detected (990.9%), enabling full monitoring
Jul 26 14:59:21 :: Pulse usage new high water mark [990.88%, 990875 usec]. Trace info: 
Pulse profiling info

Section name        | Enter Count|  Exit Count|  usec total|     pulse %|max pulse % (1 entry)
------------------------------------------------------------------------------- 
CastleGuard         |          13|          13|         695|       0.69%|               0.59%
DicknDavid          |           1|           1|         628|       0.63%|               0.63%
James               |           1|           1|         196|       0.20%|               0.20%
Main Loop           |           1|           1|      990678|     990.68%|             990.68%
Process Commands    |           1|           1|           1|       0.00%|               0.00%
Process Input       |           1|           1|           2|       0.00%|               0.00%
Process Output      |           1|           1|           1|       0.00%|               0.00%
abyssal_vortex      |           4|           4|         259|       0.26%|               0.24%
affect_update       |           1|           1|        3505|       3.50%|               3.50%
agrachdyrr          |           1|           1|         536|       0.54%|               0.54%
alandor_ferry       |           1|           1|         723|       0.72%|               0.72%
bloodaxe            |          10|          10|         778|       0.78%|               0.46%
bolthammer          |           4|           4|         412|       0.41%|               0.28%
celestial_leviathan |           1|           1|         140|       0.14%|               0.14%
celestial_sword     |           4|           4|         319|       0.32%|               0.30%
chan                |           1|           1|         238|       0.24%|               0.24%
chionthar_ferry     |           1|           1|         440|       0.44%|               0.44%
cleaning            |           5|           5|         406|       0.41%|               0.38%
cryogenicist        |           1|           1|         961|       0.96%|               0.96%
do_action           |           1|           1|         320|       0.32%|               0.32%
do_follow           |           2|           2|        3757|       3.76%|               1.99%
do_recline          |          10|          10|         501|       0.50%|               0.44%
do_rest             |           5|           5|         532|       0.53%|               0.50%
do_stand            |          29|          29|        1573|       1.57%|               0.47%
dog                 |          29|          29|         685|       0.69%|               0.52%
dracolich_mob       |           6|           6|         588|       0.59%|               0.42%
event_process       |           1|           1|        1084|       1.08%|               1.08%
flamekissed_instrument|           6|           6|         323|       0.32%|               0.29%
frostbite           |          14|          14|         488|       0.49%|               0.29%
gatehouse_guard     |           6|           6|         412|       0.41%|               0.38%
gen_board           |           1|           1|        1694|       1.69%|               1.69%
guild               |          16|          16|        1619|       1.62%|               0.57%
heartbeat           |           1|           1|      990575|     990.58%|             990.58%
imix                |           1|           1|         504|       0.50%|               0.50%
jerry               |           1|           1|         569|       0.57%|               0.57%
king_welmar         |           1|           1|         797|       0.80%|               0.80%
lich_mob            |          15|          15|         692|       0.69%|               0.58%
malevolence         |          10|          10|         322|       0.32%|               0.28%
mayor               |           1|           1|         937|       0.94%|               0.94%
mistweave           |          18|          18|         447|       0.45%|               0.27%
mobile_activity     |           1|           1|      935312|     935.31%|             935.31%
monk_glove          |           4|           4|         294|       0.29%|               0.28%
monk_glove_cold     |           8|           8|         294|       0.29%|               0.26%
msdp_update         |           1|           1|         180|       0.18%|               0.18%
naga                |           7|           7|         454|       0.45%|               0.42%
olhydra             |           1|           1|         250|       0.25%|               0.25%
peter               |           1|           1|         617|       0.62%|               0.62%
planetar            |           1|           1|         295|       0.29%|               0.29%
player_owned_shops  |           2|           2|         457|       0.46%|               0.42%
postmaster          |           1|           1|         172|       0.17%|               0.17%
proc_update         |           1|           1|       14231|      14.23%|              14.23%
questmaster         |           4|           4|         166|       0.17%|               0.13%
receptionist        |           2|           2|         541|       0.54%|               0.54%
rune_scimitar       |          12|          12|         342|       0.34%|               0.29%
shop_keeper         |           3|           3|         832|       0.83%|               0.80%
stability_boots     |           4|           4|         313|       0.31%|               0.30%
star_circlet        |           4|           4|         291|       0.29%|               0.28%
the_prisoner        |           1|           1|         728|       0.73%|               0.73%
thrym               |           1|           1|         340|       0.34%|               0.34%
tim                 |           1|           1|        2029|       2.03%|               2.03%
tom                 |           1|           1|        3411|       3.41%|               3.41%
training_master     |           1|           1|         570|       0.57%|               0.57%
tyrantseye          |           2|           2|         401|       0.40%|               0.39%
update_damage_and_effects_over_time|           1|           1|       26782|      26.78%|              26.78%
vampire_cloak       |           4|           4|         330|       0.33%|               0.31%
vampire_mob         |          22|          22|         790|       0.79%|               0.51%
vaprak_claws        |           4|           4|         353|       0.35%|               0.34%
warbow              |           2|           2|         136|       0.14%|               0.13%
willowisp           |           3|           3|         408|       0.41%|               0.39%
ymir                |           1|           1|         282|       0.28%|               0.28%
zone_update         |           1|           1|           3|       0.00%|               0.00%

Jul 26 14:59:21 :: PERFMON: Load normalized (0.4%), returning to sampling mode
Jul 26 14:59:26 :: PERFMON: High load detected (284.6%), enabling full monitoring
Jul 26 14:59:26 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 14:59:27 :: PERFMON: High load detected (859.7%), enabling full monitoring
Jul 26 14:59:27 :: PERFMON: Load normalized (23.6%), returning to sampling mode
Jul 26 14:59:32 :: PERFMON: High load detected (270.7%), enabling full monitoring
Jul 26 14:59:32 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 14:59:33 :: PERFMON: High load detected (908.8%), enabling full monitoring
Jul 26 14:59:33 :: PERFMON: Load normalized (37.9%), returning to sampling mode
Jul 26 14:59:35 :: PERFMON: High load detected (608.7%), enabling full monitoring
Jul 26 14:59:35 :: PERFMON: Load normalized (32.2%), returning to sampling mode
Jul 26 14:59:38 :: PERFMON: High load detected (245.5%), enabling full monitoring
Jul 26 14:59:38 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 14:59:39 :: PERFMON: High load detected (802.1%), enabling full monitoring
Jul 26 14:59:39 :: Pyret [147.235.211.56] has connected.
Jul 26 14:59:39 :: PERFMON: Load normalized (32.3%), returning to sampling mode
Jul 26 14:59:41 :: INFO: Loading saved object data from db for: Pyret
Jul 26 14:59:41 :: INFO: Object save header found for: Pyret
Jul 26 14:59:41 :: PERF: save_char(Pyret) took 107ms (buffer: 23830 bytes)
Jul 26 14:59:41 :: Pyret un-renting and entering game.
Jul 26 14:59:41 :: Pyret (level 30) has 233 objects (max 99999).
Jul 26 14:59:42 :: PERF: save_char(Pyret) took 237ms (buffer: 25453 bytes)
Jul 26 14:59:42 :: PERFMON: High load detected (845.0%), enabling full monitoring
Jul 26 14:59:42 :: PERFMON: Load normalized (68.5%), returning to sampling mode
Jul 26 14:59:44 :: PERFMON: High load detected (288.9%), enabling full monitoring
Jul 26 14:59:44 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 14:59:45 :: PERFMON: High load detected (1005.4%), enabling full monitoring
Jul 26 14:59:45 :: Pulse usage new high water mark [1005.35%, 1005355 usec]. Trace info: 
Pulse profiling info

Section name        | Enter Count|  Exit Count|  usec total|     pulse %|max pulse % (1 entry)
------------------------------------------------------------------------------- 
CastleGuard         |          13|          13|          82|       0.08%|               0.02%
DicknDavid          |           1|           1|          10|       0.01%|               0.01%
James               |           1|           1|           5|       0.01%|               0.01%
Main Loop           |           1|           1|     1005133|    1005.13%|            1005.13%
Process Commands    |           1|           1|           5|       0.01%|               0.01%
Process Input       |           1|           1|           3|       0.00%|               0.00%
Process Output      |           1|           1|           1|       0.00%|               0.00%
abyssal_vortex      |           2|           2|          17|       0.02%|               0.01%
affect_update       |           1|           1|        4625|       4.62%|               4.62%
agrachdyrr          |           1|           1|           9|       0.01%|               0.01%
alandor_ferry       |           1|           1|          11|       0.01%|               0.01%
bloodaxe            |          10|          10|          50|       0.05%|               0.01%
bolthammer          |           4|           4|          22|       0.02%|               0.01%
celestial_leviathan |           1|           1|           7|       0.01%|               0.01%
celestial_sword     |           6|           6|          32|       0.03%|               0.01%
cf_trainingmaster   |           3|           3|          19|       0.02%|               0.01%
chionthar_ferry     |           1|           1|           9|       0.01%|               0.01%
cleaning            |           5|           5|          25|       0.03%|               0.01%
cryogenicist        |           1|           1|           7|       0.01%|               0.01%
do_move             |           1|           1|         297|       0.30%|               0.30%
dog                 |           9|           9|          64|       0.06%|               0.01%
dracolich_mob       |           6|           6|          47|       0.05%|               0.01%
event_process       |           1|           1|       41024|      41.02%|              41.02%
flamekissed_instrument|           6|           6|          37|       0.04%|               0.01%
floating_teleport   |           1|           1|          10|       0.01%|               0.01%
frostbite           |          14|          14|          67|       0.07%|               0.01%
gatehouse_guard     |           6|           6|          33|       0.03%|               0.01%
gen_board           |           2|           2|          16|       0.02%|               0.01%
guild               |          16|          16|        1239|       1.24%|               0.12%
harpell             |           1|           1|           8|       0.01%|               0.01%
heartbeat           |           1|           1|     1005045|    1005.04%|            1005.04%
imix                |           1|           1|         151|       0.15%|               0.15%
jerry               |           1|           1|         507|       0.51%|               0.51%
king_welmar         |           1|           1|           8|       0.01%|               0.01%
lich_mob            |          14|          14|         118|       0.12%|               0.02%
malevolence         |          10|          10|          46|       0.05%|               0.01%
mayor               |           1|           1|         436|       0.44%|               0.44%
mistweave           |          18|          18|          85|       0.09%|               0.01%
mobile_activity     |           1|           1|      829857|     829.86%|             829.86%
monk_glove          |           4|           4|          28|       0.03%|               0.01%
monk_glove_cold     |           8|           8|          40|       0.04%|               0.01%
msdp_update         |           1|           1|        1267|       1.27%|               1.27%
naga                |           4|           4|          30|       0.03%|               0.01%
olhydra             |           2|           2|          18|       0.02%|               0.01%
peter               |           1|           1|           7|       0.01%|               0.01%
planetar            |           1|           1|           6|       0.01%|               0.01%
player_owned_shops  |           2|           2|          86|       0.09%|               0.05%
postmaster          |           1|           1|           7|       0.01%|               0.01%
proc_update         |           1|           1|        3616|       3.62%|               3.62%
pulse_luminari      |           1|           1|       75935|      75.94%|              75.94%
questmaster         |           3|           3|          53|       0.05%|               0.02%
receptionist        |           2|           2|          18|       0.02%|               0.01%
rune_scimitar       |          12|          12|          58|       0.06%|               0.01%
shop_keeper         |           1|           1|          20|       0.02%|               0.02%
stability_boots     |           4|           4|          22|       0.02%|               0.01%
star_circlet        |           6|           6|         336|       0.34%|               0.31%
the_prisoner        |           1|           1|           7|       0.01%|               0.01%
thrym               |           1|           1|           8|       0.01%|               0.01%
tim                 |           1|           1|           7|       0.01%|               0.01%
tom                 |           1|           1|          13|       0.01%|               0.01%
training_master     |           1|           1|           7|       0.01%|               0.01%
tyrantseye          |           2|           2|          12|       0.01%|               0.01%
update_damage_and_effects_over_time|           1|           1|       25801|      25.80%|              25.80%
vampire_cloak       |           4|           4|          25|       0.03%|               0.01%
vampire_mob         |          21|          21|         250|       0.25%|               0.11%
vaprak_claws        |           4|           4|          23|       0.02%|               0.01%
warbow              |           2|           2|          10|       0.01%|               0.01%
ymir                |           1|           1|           7|       0.01%|               0.01%
zone_update         |           1|           1|           2|       0.00%|               0.00%

Jul 26 14:59:46 :: PERFMON: Load normalized (20.3%), returning to sampling mode
Jul 26 14:59:50 :: PERFMON: High load detected (323.1%), enabling full monitoring
Jul 26 14:59:50 :: PERFMON: Load normalized (0.4%), returning to sampling mode
Jul 26 14:59:51 :: PERFMON: High load detected (878.8%), enabling full monitoring
Jul 26 14:59:51 :: PERFMON: Load normalized (1.5%), returning to sampling mode
Jul 26 14:59:56 :: PERFMON: High load detected (225.2%), enabling full monitoring
Jul 26 14:59:56 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 14:59:57 :: PERFMON: High load detected (453.3%), enabling full monitoring
Jul 26 14:59:57 :: PERFMON: Load normalized (2.1%), returning to sampling mode
Jul 26 15:00:00 :: Pyret killed by 	Dthe gi	Cthyan	Dki supreme leader	n (129000) at 	DThe As	Ctr	Dal Plane	n (129000)
Jul 26 15:00:00 :: PERF: save_char(Pyret) took 285ms (buffer: 25453 bytes)
Jul 26 15:00:00 :: PERFMON: High load detected (369.3%), enabling full monitoring
Jul 26 15:00:00 :: PERFMON: Load normalized (0.6%), returning to sampling mode
Jul 26 15:00:03 :: PERFMON: High load detected (978.7%), enabling full monitoring
Jul 26 15:00:03 :: PERFMON: Load normalized (15.5%), returning to sampling mode
Jul 26 15:00:08 :: PERFMON: High load detected (247.6%), enabling full monitoring
Jul 26 15:00:08 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 15:00:09 :: PERFMON: High load detected (880.8%), enabling full monitoring
Jul 26 15:00:09 :: PERFMON: Load normalized (1.9%), returning to sampling mode
Jul 26 15:00:14 :: PERFMON: High load detected (231.9%), enabling full monitoring
Jul 26 15:00:14 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 15:00:16 :: PERF: save_char(Pyret) took 300ms (buffer: 25453 bytes)
Jul 26 15:00:16 :: PERF: save_char(Pyret) took 276ms (buffer: 25452 bytes)
Jul 26 15:00:17 :: PERFMON: High load detected (2266.1%), enabling full monitoring
Jul 26 15:00:17 :: Pulse usage new high water mark [2266.05%, 2266053 usec]. Trace info: 
Pulse profiling info

Section name        | Enter Count|  Exit Count|  usec total|     pulse %|max pulse % (1 entry)
------------------------------------------------------------------------------- 
CastleGuard         |          14|          14|          74|       0.07%|               0.01%
Crash_save_all      |           1|           1|      665084|     665.08%|             665.08%
DicknDavid          |           1|           1|          10|       0.01%|               0.01%
House_save_all      |           1|           1|      236074|     236.07%|             236.07%
James               |           1|           1|           5|       0.01%|               0.01%
Main Loop           |           1|           1|     2265850|    2265.85%|            2265.85%
Process Commands    |           1|           1|           4|       0.00%|               0.00%
Process Input       |           1|           1|           1|       0.00%|               0.00%
Process Output      |           1|           1|           1|       0.00%|               0.00%
abyssal_vortex      |           4|           4|          26|       0.03%|               0.01%
affect_update       |           1|           1|        3427|       3.43%|               3.43%
agrachdyrr          |           1|           1|           8|       0.01%|               0.01%
alandor_ferry       |           1|           1|         490|       0.49%|               0.49%
bloodaxe            |          10|          10|          52|       0.05%|               0.01%
bolthammer          |           4|           4|          19|       0.02%|               0.01%
celestial_leviathan |           1|           1|           6|       0.01%|               0.01%
celestial_sword     |           6|           6|          31|       0.03%|               0.01%
cf_trainingmaster   |           3|           3|          20|       0.02%|               0.01%
chan                |           1|           1|           7|       0.01%|               0.01%
chionthar_ferry     |           1|           1|         486|       0.49%|               0.49%
cleaning            |           5|           5|          27|       0.03%|               0.01%
cryogenicist        |           1|           1|          56|       0.06%|               0.06%
do_action           |           2|           2|          32|       0.03%|               0.02%
do_move             |           1|           1|         352|       0.35%|               0.35%
do_say              |           1|           1|         413|       0.41%|               0.41%
dog                 |           4|           4|          39|       0.04%|               0.01%
dracolich_mob       |           6|           6|          40|       0.04%|               0.01%
event_process       |           1|           1|       44396|      44.40%|              44.40%
flamekissed_instrument|           6|           6|          31|       0.03%|               0.01%
frostbite           |          14|          14|          68|       0.07%|               0.01%
gatehouse_guard     |           6|           6|          34|       0.03%|               0.01%
guild               |          16|          16|        1105|       1.10%|               0.08%
heartbeat           |           1|           1|     2265823|    2265.82%|            2265.82%
imix                |           1|           1|           9|       0.01%|               0.01%
jerry               |           1|           1|           7|       0.01%|               0.01%
king_welmar         |           1|           1|           8|       0.01%|               0.01%
lich_mob            |          14|          14|         103|       0.10%|               0.01%
malevolence         |          10|          10|          49|       0.05%|               0.01%
mayor               |           2|           2|         205|       0.20%|               0.20%
mistweave           |          18|          18|          82|       0.08%|               0.01%
mobile_activity     |           1|           1|      894495|     894.50%|             894.50%
monk_glove          |           4|           4|          22|       0.02%|               0.01%
monk_glove_cold     |           8|           8|          42|       0.04%|               0.01%
msdp_update         |           1|           1|        1243|       1.24%|               1.24%
naga                |          10|          10|          51|       0.05%|               0.01%
peter               |           1|           1|           7|       0.01%|               0.01%
planetar            |           1|           1|           6|       0.01%|               0.01%
player_owned_shops  |           2|           2|          64|       0.06%|               0.03%
postmaster          |           1|           1|           6|       0.01%|               0.01%
proc_update         |           1|           1|        4250|       4.25%|               4.25%
pulse_luminari      |           1|           1|       64640|      64.64%|              64.64%
questmaster         |           4|           4|          55|       0.06%|               0.02%
receptionist        |           2|           2|          79|       0.08%|               0.07%
rune_scimitar       |          12|          12|          56|       0.06%|               0.01%
shop_keeper         |           7|           7|          94|       0.09%|               0.02%
stability_boots     |           4|           4|          21|       0.02%|               0.01%
star_circlet        |           6|           6|          33|       0.03%|               0.01%
the_prisoner        |           1|           1|         130|       0.13%|               0.13%
thrym               |           1|           1|          10|       0.01%|               0.01%
tim                 |           1|           1|           7|       0.01%|               0.01%
tom                 |           1|           1|          12|       0.01%|               0.01%
training_master     |           1|           1|           8|       0.01%|               0.01%
ttf_abomination     |           1|           1|         242|       0.24%|               0.24%
ttf_rotbringer      |           1|           1|         217|       0.22%|               0.22%
tyrantseye          |           2|           2|          10|       0.01%|               0.01%
update_damage_and_effects_over_time|           1|           1|       26381|      26.38%|              26.38%
vampire_cloak       |           4|           4|          22|       0.02%|               0.01%
vampire_mob         |          21|          21|         143|       0.14%|               0.01%
vaprak_claws        |           4|           4|          21|       0.02%|               0.01%
warbow              |           2|           2|          10|       0.01%|               0.01%
ymir                |           1|           1|           5|       0.01%|               0.01%
zone_update         |           1|           1|        4802|       4.80%|               4.80%

Jul 26 15:00:17 :: PERFMON: Load normalized (57.1%), returning to sampling mode
Jul 26 15:00:20 :: PERFMON: High load detected (308.0%), enabling full monitoring
Jul 26 15:00:20 :: PERFMON: Load normalized (3.2%), returning to sampling mode
==32435== Invalid read of size 4
==32435==    at 0x704197: script_driver (dg_scripts.c:2817)
==32435==    by 0x517622: greet_mtrigger (dg_triggers.c:245)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x8a76c58 is 56 bytes inside a block of size 104 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x640D73: extract_script (dg_handler.c:170)
==32435==    by 0x705DC5: process_detach (dg_scripts.c:2107)
==32435==    by 0x705DC5: script_driver (dg_scripts.c:2982)
==32435==    by 0x517622: greet_mtrigger (dg_triggers.c:245)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x4ECDDF: read_trigger (dg_db_scripts.c:89)
==32435==    by 0x4ED259: assign_triggers (dg_db_scripts.c:302)
==32435==    by 0x6BCD6C: read_mobile (db.c:3950)
==32435==    by 0x6BE692: reset_zone (db.c:4307)
==32435==    by 0x6BF6C6: boot_db (db.c:1134)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
==32435== Invalid write of size 4
==32435==    at 0x7042CA: script_driver (dg_scripts.c:2879)
==32435==    by 0x517622: greet_mtrigger (dg_triggers.c:245)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x8a76c58 is 56 bytes inside a block of size 104 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x640D73: extract_script (dg_handler.c:170)
==32435==    by 0x705DC5: process_detach (dg_scripts.c:2107)
==32435==    by 0x705DC5: script_driver (dg_scripts.c:2982)
==32435==    by 0x517622: greet_mtrigger (dg_triggers.c:245)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x4ECDDF: read_trigger (dg_db_scripts.c:89)
==32435==    by 0x4ED259: assign_triggers (dg_db_scripts.c:302)
==32435==    by 0x6BCD6C: read_mobile (db.c:3950)
==32435==    by 0x6BE692: reset_zone (db.c:4307)
==32435==    by 0x6BF6C6: boot_db (db.c:1134)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
==32435== Invalid write of size 8
==32435==    at 0x7041E0: script_driver (dg_scripts.c:3024)
==32435==    by 0x517622: greet_mtrigger (dg_triggers.c:245)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x8a76c70 is 80 bytes inside a block of size 104 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x640D73: extract_script (dg_handler.c:170)
==32435==    by 0x705DC5: process_detach (dg_scripts.c:2107)
==32435==    by 0x705DC5: script_driver (dg_scripts.c:2982)
==32435==    by 0x517622: greet_mtrigger (dg_triggers.c:245)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x4ECDDF: read_trigger (dg_db_scripts.c:89)
==32435==    by 0x4ED259: assign_triggers (dg_db_scripts.c:302)
==32435==    by 0x6BCD6C: read_mobile (db.c:3950)
==32435==    by 0x6BE692: reset_zone (db.c:4307)
==32435==    by 0x6BF6C6: boot_db (db.c:1134)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
==32435== Invalid write of size 4
==32435==    at 0x7041E8: script_driver (dg_scripts.c:3025)
==32435==    by 0x517622: greet_mtrigger (dg_triggers.c:245)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x8a76c58 is 56 bytes inside a block of size 104 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x640D73: extract_script (dg_handler.c:170)
==32435==    by 0x705DC5: process_detach (dg_scripts.c:2107)
==32435==    by 0x705DC5: script_driver (dg_scripts.c:2982)
==32435==    by 0x517622: greet_mtrigger (dg_triggers.c:245)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x4ECDDF: read_trigger (dg_db_scripts.c:89)
==32435==    by 0x4ED259: assign_triggers (dg_db_scripts.c:302)
==32435==    by 0x6BCD6C: read_mobile (db.c:3950)
==32435==    by 0x6BE692: reset_zone (db.c:4307)
==32435==    by 0x6BF6C6: boot_db (db.c:1134)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
==32435== Invalid read of size 8
==32435==    at 0x517570: greet_mtrigger (dg_triggers.c:234)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x8a76c78 is 88 bytes inside a block of size 104 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x640D73: extract_script (dg_handler.c:170)
==32435==    by 0x705DC5: process_detach (dg_scripts.c:2107)
==32435==    by 0x705DC5: script_driver (dg_scripts.c:2982)
==32435==    by 0x517622: greet_mtrigger (dg_triggers.c:245)
==32435==    by 0x5853AF: do_simple_move (act.movement.c:1995)
==32435==    by 0x5872AE: perform_move_full (act.movement.c:2140)
==32435==    by 0x4845A9: mobile_activity (mobact.c:1892)
==32435==    by 0x5BFC53: heartbeat (comm.c:1209)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x4ECDDF: read_trigger (dg_db_scripts.c:89)
==32435==    by 0x4ED259: assign_triggers (dg_db_scripts.c:302)
==32435==    by 0x6BCD6C: read_mobile (db.c:3950)
==32435==    by 0x6BE692: reset_zone (db.c:4307)
==32435==    by 0x6BF6C6: boot_db (db.c:1134)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
Jul 26 15:00:21 :: PERFMON: High load detected (1053.3%), enabling full monitoring
Jul 26 15:00:22 :: PERFMON: Load normalized (33.5%), returning to sampling mode
Jul 26 15:00:25 :: PERFMON: High load detected (157.8%), enabling full monitoring
Jul 26 15:00:25 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 15:00:26 :: PERFMON: High load detected (193.5%), enabling full monitoring
Jul 26 15:00:26 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 15:00:28 :: PERFMON: High load detected (1113.6%), enabling full monitoring
Jul 26 15:00:28 :: SYSERR: Unable to INSERT INTO pet_data: You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near ''39','30','1181','1181','37','37','22','610','16','16')' at line 1
Jul 26 15:00:28 :: QUERY: INSERT INTO pet_data (pet_data_id, owner_name, pet_name, pet_sdesc, pet_ldesc, pet_ddesc, vnum, level, hp, max_hp, str, con, dex, ac, wis, cha) VALUES(NULL,'Pyret','a young red dragon','a young red dragon','A young red dragon is standing here.\r\n','   A magnificent young red dragon, cloaked in thick red scales cascading around\r\nits body stands here.  Chromatic dragons are amongst the most fierce and the\r\ngreat reds are then again even more so.  Too young to have a great hoard of it\'s\r\nown, this young dragon, like all other dragons, hungers for treasure.\r\n',,'39','30','1181','1181','37','37','22','610','16','16')
Jul 26 15:00:28 :: PERFMON: Load normalized (41.1%), returning to sampling mode
==32435== Invalid read of size 8
==32435==    at 0x519222: timer_otrigger (dg_triggers.c:756)
==32435==    by 0x63F1E7: point_update (limits.c:2118)
==32435==    by 0x5BFB08: heartbeat (comm.c:1287)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x208b3738 is 88 bytes inside a block of size 104 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x640D73: extract_script (dg_handler.c:170)
==32435==    by 0x5D09D4: extract_obj (handler.c:2346)
==32435==    by 0x5DD038: do_opurge (dg_objcmd.c:391)
==32435==    by 0x5DDF9C: obj_command_interpreter (dg_objcmd.c:975)
==32435==    by 0x70521B: script_driver (dg_scripts.c:2993)
==32435==    by 0x519221: timer_otrigger (dg_triggers.c:760)
==32435==    by 0x63F1E7: point_update (limits.c:2118)
==32435==    by 0x5BFB08: heartbeat (comm.c:1287)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x4ECDDF: read_trigger (dg_db_scripts.c:89)
==32435==    by 0x4ED3C9: assign_triggers (dg_db_scripts.c:322)
==32435==    by 0x6BD77A: read_object (db.c:4086)
==32435==    by 0x6BE234: reset_zone (db.c:4359)
==32435==    by 0x6BF6C6: boot_db (db.c:1134)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
==32435== Invalid read of size 1
==32435==    at 0x63F06D: point_update (limits.c:2116)
==32435==    by 0x5BFB08: heartbeat (comm.c:1287)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x208b3358 is 72 bytes inside a block of size 624 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x5DD038: do_opurge (dg_objcmd.c:391)
==32435==    by 0x5DDF9C: obj_command_interpreter (dg_objcmd.c:975)
==32435==    by 0x70521B: script_driver (dg_scripts.c:2993)
==32435==    by 0x519221: timer_otrigger (dg_triggers.c:760)
==32435==    by 0x63F1E7: point_update (limits.c:2118)
==32435==    by 0x5BFB08: heartbeat (comm.c:1287)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x6BD54A: read_object (db.c:4011)
==32435==    by 0x6BE234: reset_zone (db.c:4359)
==32435==    by 0x6BF6C6: boot_db (db.c:1134)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
==32435== Invalid read of size 1
==32435==    at 0x63F076: point_update (limits.c:2149)
==32435==    by 0x5BFB08: heartbeat (comm.c:1287)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x208b3374 is 100 bytes inside a block of size 624 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x5DD038: do_opurge (dg_objcmd.c:391)
==32435==    by 0x5DDF9C: obj_command_interpreter (dg_objcmd.c:975)
==32435==    by 0x70521B: script_driver (dg_scripts.c:2993)
==32435==    by 0x519221: timer_otrigger (dg_triggers.c:760)
==32435==    by 0x63F1E7: point_update (limits.c:2118)
==32435==    by 0x5BFB08: heartbeat (comm.c:1287)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x6BD54A: read_object (db.c:4011)
==32435==    by 0x6BE234: reset_zone (db.c:4359)
==32435==    by 0x6BF6C6: boot_db (db.c:1134)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
Jul 26 15:00:30 :: PERFMON: High load detected (272.6%), enabling full monitoring
Jul 26 15:00:30 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 15:00:32 :: PERFMON: High load detected (188.4%), enabling full monitoring
Jul 26 15:00:32 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 15:00:34 :: SYSERR: Unable to INSERT INTO pet_data: You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near ''39','30','1181','1181','37','37','22','610','16','16')' at line 1
Jul 26 15:00:34 :: QUERY: INSERT INTO pet_data (pet_data_id, owner_name, pet_name, pet_sdesc, pet_ldesc, pet_ddesc, vnum, level, hp, max_hp, str, con, dex, ac, wis, cha) VALUES(NULL,'Pyret','a young red dragon','a young red dragon','A young red dragon is standing here.\r\n','   A magnificent young red dragon, cloaked in thick red scales cascading around\r\nits body stands here.  Chromatic dragons are amongst the most fierce and the\r\ngreat reds are then again even more so.  Too young to have a great hoard of it\'s\r\nown, this young dragon, like all other dragons, hungers for treasure.\r\n',,'39','30','1181','1181','37','37','22','610','16','16')
Jul 26 15:00:34 :: PERFMON: High load detected (1155.2%), enabling full monitoring
Jul 26 15:00:34 :: PERFMON: Load normalized (30.6%), returning to sampling mode
Jul 26 15:00:36 :: Pyret has quit the game.
Jul 26 15:00:36 :: SYSERR: Unable to INSERT INTO pet_data: You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near ''39','30','1181','1181','37','37','22','610','16','16')' at line 1
Jul 26 15:00:36 :: QUERY: INSERT INTO pet_data (pet_data_id, owner_name, pet_name, pet_sdesc, pet_ldesc, pet_ddesc, vnum, level, hp, max_hp, str, con, dex, ac, wis, cha) VALUES(NULL,'Pyret','a young red dragon','a young red dragon','A young red dragon is standing here.\r\n','   A magnificent young red dragon, cloaked in thick red scales cascading around\r\nits body stands here.  Chromatic dragons are amongst the most fierce and the\r\ngreat reds are then again even more so.  Too young to have a great hoard of it\'s\r\nown, this young dragon, like all other dragons, hungers for treasure.\r\n',,'39','30','1181','1181','37','37','22','610','16','16')
Jul 26 15:00:37 :: PERF: save_char(Pyret) took 51ms (buffer: 25464 bytes)
Jul 26 15:00:37 :: PERFMON: High load detected (664.2%), enabling full monitoring
Jul 26 15:00:37 :: PERFMON: Load normalized (88.9%), returning to sampling mode
Jul 26 15:00:38 :: PERFMON: High load detected (211.7%), enabling full monitoring
Jul 26 15:00:38 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 15:00:40 :: PERFMON: High load detected (1102.0%), enabling full monitoring
Jul 26 15:00:40 :: PERFMON: Load normalized (96.0%), returning to sampling mode
Jul 26 15:00:40 :: PERF: save_char(Pyret) took 62ms (buffer: 25464 bytes)
Jul 26 15:00:40 :: PERFMON: High load detected (500.5%), enabling full monitoring
Jul 26 15:00:41 :: PERFMON: Load normalized (21.6%), returning to sampling mode
Jul 26 15:00:44 :: PERFMON: High load detected (168.0%), enabling full monitoring
Jul 26 15:00:44 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 15:00:44 :: Zusuk [147.235.211.56] has connected.
Jul 26 15:00:45 :: PERFMON: High load detected (982.2%), enabling full monitoring
Jul 26 15:00:45 :: PERFMON: Load normalized (35.0%), returning to sampling mode
Jul 26 15:00:46 :: INFO: Loading saved object data from db for: Zusuk
Jul 26 15:00:46 :: INFO: Object save header found for: Zusuk
Jul 26 15:00:46 :: Zusuk retrieving crash-saved items and entering game.
Jul 26 15:00:46 :: Zusuk (level 34) has 1 object (max 99999).
Jul 26 15:00:46 :: PERFMON: High load detected (246.4%), enabling full monitoring
Jul 26 15:00:46 :: PERFMON: Load normalized (40.2%), returning to sampling mode
Jul 26 15:00:50 :: PERFMON: High load detected (257.5%), enabling full monitoring
Jul 26 15:00:50 :: PERFMON: Load normalized (58.0%), returning to sampling mode
Jul 26 15:00:51 :: PERFMON: High load detected (1020.5%), enabling full monitoring
Jul 26 15:00:52 :: PERFMON: Load normalized (29.0%), returning to sampling mode
Jul 26 15:00:54 :: (GC) Zusuk forced room 129000 to hit zusuk
Jul 26 15:00:54 :: PERFMON: High load detected (151.3%), enabling full monitoring
Jul 26 15:00:54 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 15:00:55 :: PERFMON: High load detected (159.6%), enabling full monitoring
Jul 26 15:00:55 :: PERFMON: Load normalized (0.5%), returning to sampling mode
Jul 26 15:00:56 :: PERFMON: High load detected (225.5%), enabling full monitoring
Jul 26 15:00:56 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 15:00:57 :: PERFMON: High load detected (583.6%), enabling full monitoring
Jul 26 15:00:57 :: PERFMON: Load normalized (1.0%), returning to sampling mode
==32435== Source and destination overlap in strcpy(0x1ffeff28e0, 0x1ffeff28e4)
==32435==    at 0x4C2D282: strcpy (vg_replace_strmem.c:513)
==32435==    by 0x5D3B8F: find_all_dots (handler.c:3072)
==32435==    by 0x573008: get_from_container (act.item.c:2227)
==32435==    by 0x5746B9: impl_do_get_.isra.38 (act.item.c:2417)
==32435==    by 0x574B61: do_get (act.item.c:2363)
==32435==    by 0x427C3F: dam_killed_vict (fight.c:4881)
==32435==    by 0x4367C7: damage (fight.c:5476)
==32435==    by 0x438C70: handle_successful_attack (fight.c:10590)
==32435==    by 0x4350BB: hit (fight.c:11390)
==32435==    by 0x43BB16: perform_attacks (fight.c:12258)
==32435==    by 0x43D00E: perform_violence (fight.c:13084)
==32435==    by 0x43DDD1: event_combat_round (fight.c:12580)
==32435== 
==32435== Invalid read of size 1
==32435==    at 0x572E4D: perform_get_from_container (act.item.c:2206)
==32435==    by 0x5730E5: get_from_container (act.item.c:2265)
==32435==    by 0x5746B9: impl_do_get_.isra.38 (act.item.c:2417)
==32435==    by 0x574B61: do_get (act.item.c:2363)
==32435==    by 0x427C3F: dam_killed_vict (fight.c:4881)
==32435==    by 0x4367C7: damage (fight.c:5476)
==32435==    by 0x438C70: handle_successful_attack (fight.c:10590)
==32435==    by 0x4350BB: hit (fight.c:11390)
==32435==    by 0x43BB16: perform_attacks (fight.c:12258)
==32435==    by 0x43D00E: perform_violence (fight.c:13084)
==32435==    by 0x43DDD1: event_combat_round (fight.c:12580)
==32435==    by 0x64F6F1: event_process (dg_event.c:131)
==32435==  Address 0x3f9d5768 is 72 bytes inside a block of size 624 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x5610AE: get_check_money (act.item.c:2125)
==32435==    by 0x572E43: perform_get_from_container (act.item.c:2203)
==32435==    by 0x5730E5: get_from_container (act.item.c:2265)
==32435==    by 0x5746B9: impl_do_get_.isra.38 (act.item.c:2417)
==32435==    by 0x574B61: do_get (act.item.c:2363)
==32435==    by 0x427C3F: dam_killed_vict (fight.c:4881)
==32435==    by 0x4367C7: damage (fight.c:5476)
==32435==    by 0x438C70: handle_successful_attack (fight.c:10590)
==32435==    by 0x4350BB: hit (fight.c:11390)
==32435==    by 0x43BB16: perform_attacks (fight.c:12258)
==32435==    by 0x43D00E: perform_violence (fight.c:13084)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x6B46AF: create_obj (db.c:3982)
==32435==    by 0x5D352D: create_money (handler.c:2939)
==32435==    by 0x41DEBA: make_corpse (fight.c:1806)
==32435==    by 0x422117: raw_kill (fight.c:2138)
==32435==    by 0x427628: dam_killed_vict (fight.c:4842)
==32435==    by 0x4367C7: damage (fight.c:5476)
==32435==    by 0x438C70: handle_successful_attack (fight.c:10590)
==32435==    by 0x4350BB: hit (fight.c:11390)
==32435==    by 0x43BB16: perform_attacks (fight.c:12258)
==32435==    by 0x43D00E: perform_violence (fight.c:13084)
==32435==    by 0x43DDD1: event_combat_round (fight.c:12580)
==32435== 
==32435== Invalid read of size 8
==32435==    at 0x56681C: check_room_lighting_special (act.item.c:1869)
==32435==    by 0x572CA1: perform_get_from_container (act.item.c:2218)
==32435==    by 0x5730E5: get_from_container (act.item.c:2265)
==32435==    by 0x5746B9: impl_do_get_.isra.38 (act.item.c:2417)
==32435==    by 0x574B61: do_get (act.item.c:2363)
==32435==    by 0x427C3F: dam_killed_vict (fight.c:4881)
==32435==    by 0x4367C7: damage (fight.c:5476)
==32435==    by 0x438C70: handle_successful_attack (fight.c:10590)
==32435==    by 0x4350BB: hit (fight.c:11390)
==32435==    by 0x43BB16: perform_attacks (fight.c:12258)
==32435==    by 0x43D00E: perform_violence (fight.c:13084)
==32435==    by 0x43DDD1: event_combat_round (fight.c:12580)
==32435==  Address 0x3f9d5780 is 96 bytes inside a block of size 624 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x5610AE: get_check_money (act.item.c:2125)
==32435==    by 0x572E43: perform_get_from_container (act.item.c:2203)
==32435==    by 0x5730E5: get_from_container (act.item.c:2265)
==32435==    by 0x5746B9: impl_do_get_.isra.38 (act.item.c:2417)
==32435==    by 0x574B61: do_get (act.item.c:2363)
==32435==    by 0x427C3F: dam_killed_vict (fight.c:4881)
==32435==    by 0x4367C7: damage (fight.c:5476)
==32435==    by 0x438C70: handle_successful_attack (fight.c:10590)
==32435==    by 0x4350BB: hit (fight.c:11390)
==32435==    by 0x43BB16: perform_attacks (fight.c:12258)
==32435==    by 0x43D00E: perform_violence (fight.c:13084)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x6B46AF: create_obj (db.c:3982)
==32435==    by 0x5D352D: create_money (handler.c:2939)
==32435==    by 0x41DEBA: make_corpse (fight.c:1806)
==32435==    by 0x422117: raw_kill (fight.c:2138)
==32435==    by 0x427628: dam_killed_vict (fight.c:4842)
==32435==    by 0x4367C7: damage (fight.c:5476)
==32435==    by 0x438C70: handle_successful_attack (fight.c:10590)
==32435==    by 0x4350BB: hit (fight.c:11390)
==32435==    by 0x43BB16: perform_attacks (fight.c:12258)
==32435==    by 0x43D00E: perform_violence (fight.c:13084)
==32435==    by 0x43DDD1: event_combat_round (fight.c:12580)
==32435== 
Jul 26 15:01:04 :: PERFMON: High load detected (1123.2%), enabling full monitoring
Jul 26 15:01:04 :: PERFMON: Load normalized (14.7%), returning to sampling mode
Jul 26 15:01:08 :: PERFMON: High load detected (187.3%), enabling full monitoring
Jul 26 15:01:08 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 15:01:09 :: PERFMON: High load detected (982.3%), enabling full monitoring
Jul 26 15:01:09 :: PERFMON: Load normalized (90.2%), returning to sampling mode
Jul 26 15:01:13 :: Zusuk has quit the game.
Jul 26 15:01:13 :: PERFMON: High load detected (339.2%), enabling full monitoring
Jul 26 15:01:13 :: PERFMON: Load normalized (0.4%), returning to sampling mode
Jul 26 15:01:14 :: PERFMON: High load detected (175.1%), enabling full monitoring
Jul 26 15:01:14 :: PERFMON: Load normalized (0.2%), returning to sampling mode
==32435== Invalid read of size 4
==32435==    at 0x436E85: damage (fight.c:5043)
==32435==    by 0x63FDDE: update_damage_and_effects_over_time (limits.c:2457)
==32435==    by 0x5BFB7F: heartbeat (comm.c:1257)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x2001aea0 is 16 bytes inside a block of size 17,888 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x6366C0: nanny (interpreter.c:4048)
==32435==    by 0x5C14E0: game_loop (comm.c:1012)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x635A58: nanny (interpreter.c:2424)
==32435==    by 0x5C14E0: game_loop (comm.c:1012)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435== 
Jul 26 15:01:16 :: PERFMON: High load detected (1677.0%), enabling full monitoring
Jul 26 15:01:16 :: PERFMON: Load normalized (40.4%), returning to sampling mode
Jul 26 15:01:17 :: Pyret [147.235.211.56] has connected.
Jul 26 15:01:18 :: INFO: Loading saved object data from db for: Pyret
Jul 26 15:01:18 :: INFO: Object save header found for: Pyret
Jul 26 15:01:18 :: Pyret un-renting and entering game.
Jul 26 15:01:19 :: Pyret (level 30) has 233 objects (max 99999).
Jul 26 15:01:19 :: PERF: save_char(Pyret) took 246ms (buffer: 25461 bytes)
Jul 26 15:01:19 :: PERFMON: High load detected (778.1%), enabling full monitoring
Jul 26 15:01:19 :: PERFMON: Load normalized (77.9%), returning to sampling mode
==32435== Invalid read of size 4
==32435==    at 0x436E85: damage (fight.c:5043)
==32435==    by 0x63BF5F: regen_update (limits.c:587)
==32435==    by 0x63C489: pulse_luminari (limits.c:387)
==32435==    by 0x5BFBE9: heartbeat (comm.c:1237)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Address 0x2001aea0 is 16 bytes inside a block of size 17,888 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x6366C0: nanny (interpreter.c:4048)
==32435==    by 0x5C14E0: game_loop (comm.c:1012)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x635A58: nanny (interpreter.c:2424)
==32435==    by 0x5C14E0: game_loop (comm.c:1012)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435== 
Jul 26 15:01:20 :: PERFMON: High load detected (258.1%), enabling full monitoring
Jul 26 15:01:20 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 15:01:21 :: PERFMON: High load detected (1062.4%), enabling full monitoring
Jul 26 15:01:22 :: PERFMON: Load normalized (26.6%), returning to sampling mode
Jul 26 15:01:24 :: Pyret has quit the game.
Jul 26 15:01:25 :: PERFMON: High load detected (576.3%), enabling full monitoring
Jul 26 15:01:25 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 15:01:26 :: PERFMON: High load detected (173.5%), enabling full monitoring
Jul 26 15:01:26 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 15:01:27 :: PERFMON: High load detected (990.3%), enabling full monitoring
Jul 26 15:01:27 :: PERFMON: Load normalized (34.9%), returning to sampling mode
Jul 26 15:01:28 :: PERFMON: High load detected (518.3%), enabling full monitoring
Jul 26 15:01:28 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 26 15:01:30 :: Zusuk [147.235.211.56] has connected.
Jul 26 15:01:31 :: INFO: Loading saved object data from db for: Zusuk
Jul 26 15:01:31 :: INFO: Object save header found for: Zusuk
Jul 26 15:01:31 :: Zusuk un-renting and entering game.
Jul 26 15:01:31 :: Zusuk (level 34) has 1 object (max 99999).
Jul 26 15:01:31 :: PERF: save_char(Zusuk) took 53ms (buffer: 28867 bytes)
Jul 26 15:01:31 :: PERFMON: High load detected (253.8%), enabling full monitoring
Jul 26 15:01:31 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 15:01:32 :: PERFMON: High load detected (164.5%), enabling full monitoring
Jul 26 15:01:32 :: PERFMON: Load normalized (0.2%), returning to sampling mode
Jul 26 15:01:33 :: PERFMON: High load detected (935.5%), enabling full monitoring
Jul 26 15:01:33 :: (GC) Shutdown by Zusuk (KILLSCRIPT).
Jul 26 15:01:34 :: PERF: save_char(Zusuk) took 90ms (buffer: 28866 bytes)
Jul 26 15:01:34 :: Closing all sockets.
Jul 26 15:01:34 :: Closing link to: Zusuk.
Jul 26 15:01:34 :: Saving current MUD time.
Jul 26 15:01:34 :: Normal termination of game.
Jul 26 15:01:34 :: Dev port set in utils.h to: 4101.
Jul 26 15:01:34 :: Clearing game world.
==32435== Invalid read of size 8
==32435==    at 0x5BDA90: act (comm.c:3679)
==32435==    by 0x595868: stop_follower (utils.c:3124)
==32435==    by 0x6B7D87: destroy_db (db.c:753)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Address 0x24c5cf10 is 17,728 bytes inside a block of size 17,888 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x6B7D8F: destroy_db (db.c:754)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x6BCBED: read_mobile (db.c:3886)
==32435==    by 0x6BE692: reset_zone (db.c:4307)
==32435==    by 0x6BF99B: zone_update (db.c:4177)
==32435==    by 0x5BFD57: heartbeat (comm.c:1198)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435== 
==32435== Invalid read of size 8
==32435==    at 0x5BDC98: act (comm.c:3679)
==32435==    by 0x595868: stop_follower (utils.c:3124)
==32435==    by 0x6B7D87: destroy_db (db.c:753)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Address 0x24c5cf28 is 17,752 bytes inside a block of size 17,888 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x6B7D8F: destroy_db (db.c:754)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x6BCBED: read_mobile (db.c:3886)
==32435==    by 0x6BE692: reset_zone (db.c:4307)
==32435==    by 0x6BF99B: zone_update (db.c:4177)
==32435==    by 0x5BFD57: heartbeat (comm.c:1198)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435== 
==32435== Invalid read of size 8
==32435==    at 0x5BDBF8: act (comm.c:3675)
==32435==    by 0x595868: stop_follower (utils.c:3124)
==32435==    by 0x6B7D87: destroy_db (db.c:753)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Address 0x24c5cf38 is 17,768 bytes inside a block of size 17,888 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x6B7D8F: destroy_db (db.c:754)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x6BCBED: read_mobile (db.c:3886)
==32435==    by 0x6BE692: reset_zone (db.c:4307)
==32435==    by 0x6BF99B: zone_update (db.c:4177)
==32435==    by 0x5BFD57: heartbeat (comm.c:1198)
==32435==    by 0x5C09F4: game_loop (comm.c:1091)
==32435==    by 0x404788: init_game (comm.c:599)
==32435==    by 0x404788: main (comm.c:404)
==32435== 
==32435== Invalid read of size 8
==32435==    at 0x5B5843: next_in_list (lists.c:568)
==32435==    by 0x4C9FAB: free_craft (crafts.c:89)
==32435==    by 0x6B83D6: destroy_db (db.c:958)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Address 0x2040ae78 is 8 bytes inside a block of size 24 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x4C9F9B: free_craft (crafts.c:91)
==32435==    by 0x6B83D6: destroy_db (db.c:958)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x5B5662: create_item (lists.c:376)
==32435==    by 0x5B56B0: add_to_list (lists.c:441)
==32435==    by 0x4CA315: load_crafts (crafts.c:166)
==32435==    by 0x6BF808: boot_db (db.c:1100)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
==32435== Invalid read of size 8
==32435==    at 0x5B5843: next_in_list (lists.c:568)
==32435==    by 0x5B5A89: simple_list (lists.c:677)
==32435==    by 0x6B83E2: destroy_db (db.c:955)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Address 0x2040ba08 is 8 bytes inside a block of size 24 free'd
==32435==    at 0x4C2B06D: free (vg_replace_malloc.c:540)
==32435==    by 0x6B83CE: destroy_db (db.c:957)
==32435==    by 0x4043BD: main (comm.c:409)
==32435==  Block was alloc'd at
==32435==    at 0x4C2C089: calloc (vg_replace_malloc.c:762)
==32435==    by 0x5B5662: create_item (lists.c:376)
==32435==    by 0x5B56B0: add_to_list (lists.c:441)
==32435==    by 0x4CA2C6: load_crafts (crafts.c:142)
==32435==    by 0x6BF808: boot_db (db.c:1100)
==32435==    by 0x404694: init_game (comm.c:584)
==32435==    by 0x404694: main (comm.c:404)
==32435== 
Jul 26 15:01:37 :: WARNING: Attempting to merge iterator to empty list.
Jul 26 15:01:37 :: Clearing other memory.
Jul 26 15:01:37 :: Done.
Using file descriptor for logging.
==32435== 
==32435== HEAP SUMMARY:
==32435==     in use at exit: 43,412,575 bytes in 177,570 blocks
==32435==   total heap usage: 1,165,103 allocs, 987,533 frees, 951,584,071 bytes allocated
==32435== 
==32435== LEAK SUMMARY:
==32435==    definitely lost: 8,773,834 bytes in 113,551 blocks
==32435==    indirectly lost: 32,032,036 bytes in 53,871 blocks
==32435==      possibly lost: 2,153,844 bytes in 128 blocks
==32435==    still reachable: 452,861 bytes in 10,020 blocks
==32435==         suppressed: 0 bytes in 0 blocks
==32435== Rerun with --leak-check=full to see details of leaked memory
==32435== 
==32435== For lists of detected and suppressed errors, rerun with: -s
==32435== ERROR SUMMARY: 11448 errors from 18 contexts (suppressed: 0 from 0)
