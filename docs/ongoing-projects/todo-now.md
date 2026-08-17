# Production Health Work Remaining

## Budget player autosaves across pulses

The production heartbeat still calls `Crash_save_all()` and `House_save_all()`
synchronously after each autosave interval. `Crash_save_incremental()` and per-player
object/character save timing exist, but the incremental path has no production caller.
Increasing the autosave interval reduces frequency without removing the main-loop stall.

Work remaining:

- Replace the heartbeat's all-player crash save with an incremental scheduler bounded by
  both elapsed time and save count per pulse.
- Preserve atomic, durable object and character saves, eventual completion for every dirty
  connected player, and safe cursor behavior when descriptors disconnect or reorder.
- Measure `House_save_all()` separately and budget or defer it if it can exceed the remaining
  pulse allowance.
- Add production-linked coverage for cursor continuation, disconnects, save failures,
  fairness, and completion across multiple pulses.
- Validate under representative development load that autosave does not create a pulse over
  500 ms and normally remains within the 100 ms pulse budget.

## Close the anonymous-memory growth finding

The built-in PERFMON inventory and `scripts/process-memory/monitor_process_memory.sh` now
provide the required telemetry, but there is no retained representative time series proving
that anonymous RSS plateaus or identifying and bounding continued growth.

Work remaining:

- Record a long-running time series across idle periods, normal player activity,
  wilderness use, combat, saves, and copyovers on the current deployed image.
- Correlate RSS, anonymous RSS, and allocator growth with mobiles, objects, affects,
  affected characters, NPC followers, charmed NPCs, events, and player population.
- If anonymous RSS continues rising while live-entity counts remain stable, capture a
  development allocation trace for the corresponding workload and repair the owning path.
- Define and verify an acceptable steady-state bound and operating headroom for the normal
  process lifetime.

This item is complete only when representative evidence shows a stable plateau or documents
an identified, justified, and operationally safe bound.

### Latest `perfmon all` capture

```text
                     perfmon all
                     Avg         Min         Max
  1 Pulse:        20.82%      20.82%      20.82%
 10 Pulses:        5.30%       1.02%      20.82%
 60 Seconds:       6.11%       0.82%    1366.53%
 60 Minutes:       6.06%       0.72%    1603.01%
  3 Hours:         6.35%       0.00%    4196.80%
Max pulse:      4196.80
Over    10%:      6.25% (6786)
Over    30%:      3.35% (3633)
Over    50%:      0.82% (885)
Over    70%:      0.46% (502)
Over    90%:      0.35% (375)
Over   100%:      0.32% (343)
Over   250%:      0.22% (234)
Over   500%:      0.17% (188)
Over  1000%:      0.17% (185)
Over  2500%:      0.00% (1)
Cumulative profiling info
Section name            |    Calls|  Total usec| Total %|  Avg usec|Median usec|  P95 usec|  P99
usec|  Max usec|Samples stored/seen
----------------------------------------------------------------------------------------------------------------------------------------
do_mload                |       12|         114|    0.00%|      9.50|      0.00|      0.00|
0.00|        15|      0/0
do_wield                |        1|         535|    0.00%|    535.00|      0.00|      0.00|
0.00|       535|      0/0
do_rescue               |        7|       32390|    0.00%|   4627.14|      0.00|      0.00|
0.00|      5530|      0/0
do_wear                 |       10|        4907|    0.00%|    490.70|      0.00|      0.00|
0.00|      2082|      0/0
do_follow               |       36|      138778|    0.00%|   3854.94|      0.00|      0.00|
0.00|      4249|      0/0
do_gen_door             |     4725|       44049|    0.00%|      9.32|      0.00|      0.00|
0.00|        97|      0/0
do_mpurge               |       50|      148719|    0.00%|   2974.38|      0.00|
[Page 1/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (1/15) ]
0.00|      0.00|      4445|      0/0
questmaster             |     6892|       13404|    0.00%|      1.94|      0.00|      0.00|
0.00|        72|      0/0
do_sit                  |       33|          74|    0.00%|      2.24|      0.00|      0.00|
0.00|         8|      0/0
do_group                |       16|         433|    0.00%|     27.06|      0.00|      0.00|
0.00|       119|      0/0
do_action               |    12409|      131459|    0.00%|     10.59|      0.00|      0.00|
0.00|      4915|      0/0
do_mteleport            |     1730|     6728388|    0.06%|   3889.24|      0.00|      0.00|
0.00|     30084|      0/0
do_put                  |       25|        1380|    0.00%|     55.20|      0.00|      0.00|
0.00|       155|      0/0
do_stand                |      197|       41358|    0.00%|    209.94|      0.00|      0.00|
0.00|     40454|      0/0
Main Loop               |   108563|   688655323|    6.19%|   6343.37|      0.00|      0.00|
0.00|   4196787|      0/0
Process Input           |   108564|       59672|    0.00%|      0.55|      0.00|      0.00|
0.00|       501|      0/0
Process Commands        |   108563|     3802304|    0.03%|     35.02|      0.00|      0.00|
0.00|    335475|      0/0
Process Output          |   108563|      829787|    0.01%|      7.64|      0.00|      0.00|
0.00|       714|      0/0
heartbeat               |   111280|   678441577|    6.10%|   6096.71|      0.00|      0.00|
0.00|   4196740|      0/0
event_process           |   111280|    46748640|    0.42%|    420.10|      2.00|    241.00|
1491.34|    764956|  16384/111280
shop_keeper             |    30340|      154372|    0.00%|      5.09|      0.00|      0.00|
0.00|       139|      0/0
mobile_activity         |   111280|   199289448|    1.79%|   1790.88|      0.00|      0.00|
0.00|    244827|      0/0
janitor                 |    96124|       26173|    0.00%|      0.27|      0.00|      0.00|
0.00|        56|      0/0
pet_shops               |      969|        1656|    0.00%|      1.71|      0.00|      0.00|
0.00|        39|      0/0
do_say                  |    91789|      960031|    0.01%|     10.46|      0.00|      0.00|
0.00|      7830|      0/0
extract_pending_chars   |   111280|      946091|    0.01%|      8.50|      1.00|
[Page 2/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (2/15) ]
1.00|      1.00|     36776|  16384/111280
extract.last_attacker   |     1035|          91|    0.00%|      0.09|      0.00|      0.00|
0.00|         1|      0/0
extract.relationships   |     1035|         633|    0.00%|      0.61|      0.00|      0.00|
0.00|       159|      0/0
extract.assets          |     1035|       93625|    0.00%|     90.46|      0.00|      0.00|
0.00|      1164|      0/0
extract.combat_refs     |     1035|         405|    0.00%|      0.39|      0.00|      0.00|
0.00|       167|      0/0
extract.world_remove    |     1035|       25156|    0.00%|     24.31|      0.00|      0.00|
0.00|     18328|      0/0
extract.events          |     1035|         273|    0.00%|      0.26|      0.00|      0.00|
0.00|       151|      0/0
extract.finalize        |     1035|        1159|    0.00%|      1.12|      0.00|      0.00|
0.00|        21|      0/0
vampire_mob             |    44510|       11788|    0.00%|      0.26|      0.00|      0.00|
0.00|        63|      0/0
cf_trainingmaster       |     1884|         633|    0.00%|      0.34|      0.00|      0.00|
0.00|        25|      0/0
dog                     |     9106|        3119|    0.00%|      0.34|      0.00|      0.00|
0.00|        58|      0/0
guild                   |    37726|      130452|    0.00%|      3.46|      0.00|      0.00|
0.00|        73|      0/0
do_rest                 |      157|          59|    0.00%|      0.38|      0.00|      0.00|
0.00|         4|      0/0
vessel_tick             |    22256|     3262592|    0.03%|    146.59|     64.00|    355.00|
417.00|      3071|  16384/22256
vessel_autopilot        |    22256|     2405407|    0.02%|    108.08|     14.00|    308.00|
356.00|       572|  16384/22256
vessel_hunters          |    22256|       50877|    0.00%|      2.29|      2.00|      3.00|
4.00|        97|  16384/22256
vessel_combat           |    22256|       55567|    0.00%|      2.50|      2.00|      4.00|
4.00|        89|  16384/22256
vessel_events           |    22256|        4651|    0.00%|      0.21|      0.00|      1.00|
1.00|        25|  16384/22256
vessel_crew_wages       |    22256|       45725|    0.00%|      2.05|      2.00|      3.00|
4.00|        65|  16384/22256
vessel_upkeep           |    22256|       40140|    0.00%|      1.80|      2.00|
[Page 3/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (3/15) ]
3.00|      4.00|        69|  16384/22256
vessel_trade            |    22256|       14485|    0.00%|      0.65|      0.00|      1.00|
1.00|       456|  16384/22256
vessel_weather          |    22256|       16857|    0.00%|      0.76|      1.00|      1.00|
6.00|        95|  16384/22256
vessel_encounters       |    22256|        4754|    0.00%|      0.21|      0.00|      1.00|
1.00|        24|  16384/22256
vessel_msdp             |    22256|      587156|    0.01%|     26.38|     26.00|     41.00|
59.00|      3056|  16384/22256
receptionist            |    39226|      101787|    0.00%|      2.59|      0.00|      0.00|
0.00|       121|      0/0
do_sleep                |       18|          11|    0.00%|      0.61|      0.00|      0.00|
0.00|         4|      0/0
msdp_update             |    11128|    11197951|    0.10%|   1006.29|      0.00|      0.00|
0.00|      1802|      0/0
rol_ship_navigator      |    18790|        9565|    0.00%|      0.51|      0.00|      0.00|
0.00|        62|      0/0
do_recline              |       11|           2|    0.00%|      0.18|      0.00|      0.00|
0.00|         1|      0/0
do_mat                  |       62|        4834|    0.00%|     77.97|      0.00|      0.00|
0.00|       251|      0/0
zone_update             |     3709|     2276048|    0.02%|    613.66|      0.00|      0.00|
0.00|    296148|      0/0
do_gen_comm             |      206|       47952|    0.00%|    232.78|      0.00|      0.00|
0.00|       462|      0/0
planetar                |     1864|         629|    0.00%|      0.34|      0.00|      0.00|
0.00|        52|      0/0
ymir                    |     1943|         232|    0.00%|      0.12|      0.00|      0.00|
0.00|         7|      0/0
gatehouse_guard         |    11244|        1837|    0.00%|      0.16|      0.00|      0.00|
0.00|        43|      0/0
thrym                   |     1855|         472|    0.00%|      0.25|      0.00|      0.00|
0.00|        48|      0/0
lich_mob                |    26245|       14609|    0.00%|      0.56|      0.00|      0.00|
0.00|        87|      0/0
abyssal_vortex          |     6333|         977|    0.00%|      0.15|      0.00|      0.00|
0.00|        22|      0/0
dracolich_mob           |    11404|        4069|    0.00%|      0.36|      0.00|
[Page 4/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (4/15) ]
0.00|      0.00|        33|      0/0
olhydra                 |      288|          80|    0.00%|      0.28|      0.00|      0.00|
0.00|         1|      0/0
gromph                  |       74|          23|    0.00%|      0.31|      0.00|      0.00|
0.00|         1|      0/0
agrachdyrr              |     1855|         888|    0.00%|      0.48|      0.00|      0.00|
0.00|         2|      0/0
naga                    |     2227|         330|    0.00%|      0.15|      0.00|      0.00|
0.00|         1|      0/0
the_prisoner            |     1854|        1116|    0.00%|      0.60|      0.00|      0.00|
0.00|         2|      0/0
planewalker             |        1|           0|    0.00%|      0.00|      0.00|      0.00|
0.00|         0|      0/0
imix                    |     2112|         989|    0.00%|      0.47|      0.00|      0.00|
0.00|        56|      0/0
do_mrolwalkto           |    16371|     9726440|    0.09%|    594.13|      0.00|      0.00|
0.00|      3840|      0/0
pulse_luminari          |     2225|    45271455|    0.41%|  20346.72|      0.00|      0.00|
0.00|     35091|      0/0
chionthar_ferry         |     1864|      385281|    0.00%|    206.70|      0.00|      0.00|
0.00|       966|      0/0
postmaster              |     2117|        1125|    0.00%|      0.53|      0.00|      0.00|
0.00|         2|      0/0
do_look                 |      176|       10295|    0.00%|     58.49|      0.00|      0.00|
0.00|       432|      0/0
CastleGuard             |    24852|        5844|    0.00%|      0.24|      0.00|      0.00|
0.00|        41|      0/0
jerry                   |     1865|        1130|    0.00%|      0.61|      0.00|      0.00|
0.00|         8|      0/0
DicknDavid              |     3726|        1461|    0.00%|      0.39|      0.00|      0.00|
0.00|         2|      0/0
tom                     |     2083|        6765|    0.00%|      3.25|      0.00|      0.00|
0.00|      4051|      0/0
tim                     |     2023|        3749|    0.00%|      1.85|      0.00|      0.00|
0.00|      3330|      0/0
cleaning                |     9804|        1778|    0.00%|      0.18|      0.00|      0.00|
0.00|        26|      0/0
James                   |     1854|         479|    0.00%|      0.26|      0.00|
[Page 5/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (5/15) ]
0.00|      0.00|        44|      0/0
training_master         |     1864|        1806|    0.00%|      0.97|      0.00|      0.00|
0.00|        31|      0/0
peter                   |     1954|         680|    0.00%|      0.35|      0.00|      0.00|
0.00|        29|      0/0
king_welmar             |     1872|        1066|    0.00%|      0.57|      0.00|      0.00|
0.00|        31|      0/0
celestial_leviathan     |     1938|         391|    0.00%|      0.20|      0.00|      0.00|
0.00|        41|      0/0
mayor                   |     1916|        1508|    0.00%|      0.79|      0.00|      0.00|
0.00|        99|      0/0
cryogenicist            |     1854|        4172|    0.00%|      2.25|      0.00|      0.00|
0.00|        67|      0/0
proc_update             |     1854|    10346166|    0.09%|   5580.46|      0.00|      0.00|
0.00|    245595|      0/0
star_circlet            |     9270|        2520|    0.00%|      0.27|      0.00|      0.00|
0.00|        20|      0/0
flamekissed_instrument  |    14832|        3973|    0.00%|      0.27|      0.00|      0.00|
0.00|        34|      0/0
rune_scimitar           |    11676|     4383646|    0.04%|    375.44|      0.00|      0.00|
0.00|    461818|      0/0
mistweave               |     5562|         774|    0.00%|      0.14|      0.00|      0.00|
0.00|        28|      0/0
celestial_sword         |     9270|         807|    0.00%|      0.09|      0.00|      0.00|
0.00|         1|      0/0
speed_gaunts            |     7416|        3256|    0.00%|      0.44|      0.00|      0.00|
0.00|        25|      0/0
stability_boots         |     9275|         933|    0.00%|      0.10|      0.00|      0.00|
0.00|         2|      0/0
spikeshield             |     1854|         310|    0.00%|      0.17|      0.00|      0.00|
0.00|         1|      0/0
malevolence             |     6745|        1557|    0.00%|      0.23|      0.00|      0.00|
0.00|        21|      0/0
monk_glove_cold         |     7416|        1695|    0.00%|      0.23|      0.00|      0.00|
0.00|         3|      0/0
frostbite               |     3708|        1356|    0.00%|      0.37|      0.00|      0.00|
0.00|         3|      0/0
alandor_ferry           |     1854|      469881|    0.00%|    253.44|      0.00|
[Page 6/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (6/15) ]
0.00|      0.00|       855|      0/0
tyrantseye              |     3708|        1229|    0.00%|      0.33|      0.00|      0.00|
0.00|        23|      0/0
bloodaxe                |     3779|        1688|    0.00%|      0.45|      0.00|      0.00|
0.00|        26|      0/0
bolthammer              |     3708|         998|    0.00%|      0.27|      0.00|      0.00|
0.00|        23|      0/0
affect_update           |     1854|     7877044|    0.07%|   4248.68|      0.00|      0.00|
0.00|      9137|      0/0
update_damage_and_effect|     1854|    19451197|    0.17%|  10491.48|      0.00|      0.00|
0.00|     17039|      0/0
quicksand               |     1078|         372|    0.00%|      0.35|      0.00|      0.00|
0.00|        47|      0/0
gen_board               |      984|        1386|    0.00%|      1.41|      0.00|      0.00|
0.00|       115|      0/0
do_mhunt                |      373|     2412060|    0.02%|   6466.65|      0.00|      0.00|
0.00|     12245|      0/0
do_move                 |     9278|      358614|    0.00%|     38.65|      0.00|      0.00|
0.00|     10700|      0/0
willowisp               |     2319|        1690|    0.00%|      0.73|      0.00|      0.00|
0.00|        43|      0/0
do_arcanemark           |        5|          27|    0.00%|      5.40|      0.00|      0.00|
0.00|         7|      0/0
do_echo                 |     5138|        7311|    0.00%|      1.42|      0.00|      0.00|
0.00|        56|      0/0
do_mforce               |      420|       29290|    0.00%|     69.74|      0.00|      0.00|
0.00|      8233|      0/0
do_flee                 |      398|       25560|    0.00%|     64.22|      0.00|      0.00|
0.00|      8225|      0/0
player_owned_shops      |     3706|        5564|    0.00%|      1.50|      0.00|      0.00|
0.00|        47|      0/0
script_trigger_check    |      856|    15142487|    0.14%|  17689.82|      0.00|      0.00|
0.00|     45353|      0/0
do_mecho                |    15972|       94695|    0.00%|      5.93|      0.00|      0.00|
0.00|       146|      0/0
floating_teleport       |     1303|         394|    0.00%|      0.30|      0.00|      0.00|
0.00|         2|      0/0
fog_dagger              |      131|          69|    0.00%|      0.53|      0.00|
[Page 7/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (7/15) ]
0.00|      0.00|         1|      0/0
rol_ship_exit           |      136|          88|    0.00%|      0.65|      0.00|      0.00|
0.00|         2|      0/0
rol_ship_lookout        |      855|         635|    0.00%|      0.74|      0.00|      0.00|
0.00|        32|      0/0
rol_ship                |      181|         146|    0.00%|      0.81|      0.00|      0.00|
0.00|        36|      0/0
harpell                 |      274|         124|    0.00%|      0.45|      0.00|      0.00|
0.00|        43|      0/0
do_assist               |        2|        7713|    0.00%|   3856.50|      0.00|      0.00|
0.00|      3959|      0/0
rol_ship_control        |      240|          75|    0.00%|      0.31|      0.00|      0.00|
0.00|         1|      0/0
bazaar                  |      191|         997|    0.00%|      5.22|      0.00|      0.00|
0.00|        46|      0/0
do_inventory            |       30|        1440|    0.00%|     48.00|      0.00|      0.00|
0.00|       156|      0/0
magma                   |      198|          75|    0.00%|      0.38|      0.00|      0.00|
0.00|         1|      0/0
hellfire                |      129|         148|    0.00%|      1.15|      0.00|      0.00|
0.00|         2|      0/0
do_gen_cast             |      184|      296266|    0.00%|   1610.14|      0.00|      0.00|
0.00|     16091|      0/0
guild_guard             |      724|        1329|    0.00%|      1.84|      0.00|      0.00|
0.00|         4|      0/0
old skool tick          |      148|     3457552|    0.03%|  23361.84|      0.00|      0.00|
0.00|     31155|      0/0
vessel_schedules        |      148|       52452|    0.00%|    354.41|    362.00|    472.55|
513.00|       537|    148/148
flamingwhip             |        1|           0|    0.00%|      0.00|      0.00|      0.00|
0.00|         0|      0/0
greatsword              |      192|          65|    0.00%|      0.34|      0.00|      0.00|
0.00|         1|      0/0
helmblade               |       57|          21|    0.00%|      0.37|      0.00|      0.00|
0.00|         1|      0/0
do_stat                 |        5|       16482|    0.00%|   3296.40|      0.00|      0.00|
0.00|      4137|      0/0
giantslayer             |        3|           3|    0.00%|      1.00|      0.00|
[Page 8/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (8/15) ]
0.00|      0.00|         1|      0/0
valkyrie_sword          |        1|           0|    0.00%|      0.00|      0.00|      0.00|
0.00|         0|      0/0
do_happyhour            |        9|        1784|    0.00%|    198.22|      0.00|      0.00|
0.00|      1729|      0/0
do_help                 |       48|      470195|    0.00%|   9795.73|      0.00|      0.00|
0.00|     15135|      0/0
do_mrolzoneecho         |       31|         118|    0.00%|      3.81|      0.00|      0.00|
0.00|         8|      0/0
do_msend                |      290|         954|    0.00%|      3.29|      0.00|      0.00|
0.00|         7|      0/0
do_mechoaround          |      290|        1120|    0.00%|      3.86|      0.00|      0.00|
0.00|        62|      0/0
feybranche              |       16|           7|    0.00%|      0.44|      0.00|      0.00|
0.00|         1|      0/0
duergar_guard           |       62|          39|    0.00%|      0.63|      0.00|      0.00|
0.00|         7|      0/0
ymir_cloak              |       74|          64|    0.00%|      0.86|      0.00|      0.00|
0.00|         2|      0/0
ttf_abomination         |      238|          62|    0.00%|      0.26|      0.00|      0.00|
0.00|         6|      0/0
ttf_rotbringer          |      238|          52|    0.00%|      0.22|      0.00|      0.00|
0.00|         1|      0/0
do_affects              |       40|        1083|    0.00%|     27.07|      0.00|      0.00|
0.00|        70|      0/0
do_who                  |       13|        1197|    0.00%|     92.08|      0.00|      0.00|
0.00|       114|      0/0
do_save                 |        4|     1121320|    0.01%| 280330.00|      0.00|      0.00|
0.00|    335407|      0/0
do_gen_tog              |        3|          30|    0.00%|     10.00|      0.00|      0.00|
0.00|        22|      0/0
do_gen_preparation      |       15|         620|    0.00%|     41.33|      0.00|      0.00|
0.00|        95|      0/0
ttf_monstrosity         |      121|          35|    0.00%|      0.29|      0.00|      0.00|
0.00|         1|      0/0
wizard_library          |       14|          16|    0.00%|      1.14|      0.00|      0.00|
0.00|         2|      0/0
do_drop                 |        4|         108|    0.00%|     27.00|      0.00|
[Page 9/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (9/15) ]
0.00|      0.00|        54|      0/0
do_get                  |      218|       13925|    0.00%|     63.88|      0.00|      0.00|
0.00|       366|      0/0
do_equipment            |       11|         433|    0.00%|     39.36|      0.00|      0.00|
0.00|        70|      0/0
do_remove_board         |        3|        1401|    0.00%|    467.00|      0.00|      0.00|
0.00|       525|      0/0
do_remove               |        3|        1394|    0.00%|    464.67|      0.00|      0.00|
0.00|       521|      0/0
planetar_sword          |        3|           0|    0.00%|      0.00|      0.00|      0.00|
0.00|         0|      0/0
do_score                |        7|         573|    0.00%|     81.86|      0.00|      0.00|
0.00|        93|      0/0
do_consider             |        4|       16241|    0.00%|   4060.25|      0.00|      0.00|
0.00|      4278|      0/0
do_spells               |       14|       31034|    0.00%|   2216.71|      0.00|      0.00|
0.00|      3235|      0/0
do_split                |       41|         127|    0.00%|      3.10|      0.00|      0.00|
0.00|        22|      0/0
do_sac                  |       85|       15666|    0.00%|    184.31|      0.00|      0.00|
0.00|      3618|      0/0
do_scan                 |       17|         544|    0.00%|     32.00|      0.00|      0.00|
0.00|        70|      0/0
do_exits                |        4|          50|    0.00%|     12.50|      0.00|      0.00|
0.00|        14|      0/0
shobalar                |       39|          15|    0.00%|      0.38|      0.00|      0.00|
0.00|         1|      0/0
do_gain                 |        5|      120965|    0.00%|  24193.00|      0.00|      0.00|
0.00|     27882|      0/0
do_study                |        8|         380|    0.00%|     47.50|      0.00|      0.00|
0.00|        87|      0/0
Crash_save_all          |       12|     5493275|    0.05%| 457772.92|      0.00|      0.00|
0.00|   2867629|      0/0
House_save_all          |       12|       60373|    0.00%|   5031.08|      0.00|      0.00|
0.00|     60314|      0/0
do_dismiss              |        6|       40359|    0.00%|   6726.50|      0.00|      0.00|
0.00|      9974|      0/0
do_respec               |        1|       52639|    0.00%|  52639.00|      0.00|
[Page 10/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (10/15) ]
0.00|      0.00|     52639|      0/0
do_mdoor                |       24|          98|    0.00%|      4.08|      0.00|      0.00|
0.00|         6|      0/0
secomber_guard          |       22|          14|    0.00%|      0.64|      0.00|      0.00|
0.00|         2|      0/0
do_wake                 |        7|          10|    0.00%|      1.43|      0.00|      0.00|
0.00|         4|      0/0
crafting_kit            |      422|         415|    0.00%|      0.98|      0.00|      0.00|
0.00|         7|      0/0
faction_mission         |       56|       37302|    0.00%|    666.11|      0.00|      0.00|
0.00|     12749|      0/0
do_history              |        5|         140|    0.00%|     28.00|      0.00|      0.00|
0.00|        74|      0/0
do_date                 |        1|           5|    0.00%|      5.00|      0.00|      0.00|
0.00|         5|      0/0
do_missions             |        2|          11|    0.00%|      5.50|      0.00|      0.00|
0.00|         6|      0/0
do_order                |        1|        4141|    0.00%|   4141.00|      0.00|      0.00|
0.00|      4141|      0/0
do_call                 |        2|        4223|    0.00%|   2111.50|      0.00|      0.00|
0.00|      2472|      0/0
do_rage                 |       22|       32550|    0.00%|   1479.55|      0.00|      0.00|
0.00|      2127|      0/0
do_hit                  |       37|      155814|    0.00%|   4211.19|      0.00|      0.00|
0.00|      5418|      0/0
do_harvest              |       10|       10016|    0.00%|   1001.60|      0.00|      0.00|
0.00|      4977|      0/0
do_walkto               |        5|          30|    0.00%|      6.00|      0.00|      0.00|
0.00|         7|      0/0
do_walkto_city          |        5|          25|    0.00%|      5.00|      0.00|      0.00|
0.00|         6|      0/0
do_grapple              |        4|         218|    0.00%|     54.50|      0.00|      0.00|
0.00|        70|      0/0
tia_rapier              |        1|           1|    0.00%|      1.00|      0.00|      0.00|
0.00|         1|      0/0
do_pin                  |        3|          68|    0.00%|     22.67|      0.00|      0.00|
0.00|        23|      0/0
do_spec_comm            |        3|       12478|    0.00%|   4159.33|      0.00|
[Page 11/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (11/15) ]
0.00|      0.00|      4350|      0/0
do_wiznet               |        4|         356|    0.00%|     89.00|      0.00|      0.00|
0.00|       121|      0/0
do_carriage             |        1|       12011|    0.00%|  12011.00|      0.00|      0.00|
0.00|     12011|      0/0
battlemaze_guard        |        7|           1|    0.00%|      0.14|      0.00|      0.00|
0.00|         1|      0/0
do_levels               |        1|          47|    0.00%|     47.00|      0.00|      0.00|
0.00|        47|      0/0
do_lore                 |        7|         813|    0.00%|    116.14|      0.00|      0.00|
0.00|       165|      0/0
do_enter                |        1|         134|    0.00%|    134.00|      0.00|      0.00|
0.00|       134|      0/0
do_vstat                |        1|        4048|    0.00%|   4048.00|      0.00|      0.00|
0.00|      4048|      0/0
do_oasis_list           |        1|         707|    0.00%|    707.00|      0.00|      0.00|
0.00|       707|      0/0
do_checkloadstatus      |        1|         669|    0.00%|    669.00|      0.00|      0.00|
0.00|       669|      0/0
do_goto                 |        2|         234|    0.00%|    117.00|      0.00|      0.00|
0.00|       129|      0/0
do_oasis_zedit          |        2|         257|    0.00%|    128.50|      0.00|      0.00|
0.00|       144|      0/0
do_qref                 |        2|        2309|    0.00%|   1154.50|      0.00|      0.00|
0.00|      1257|      0/0
do_feats                |        2|         483|    0.00%|    241.50|      0.00|      0.00|
0.00|       268|      0/0
do_file                 |        2|         429|    0.00%|    214.50|      0.00|      0.00|
0.00|       350|      0/0
do_changelog            |        1|         485|    0.00%|    485.00|      0.00|      0.00|
0.00|       485|      0/0
do_ibt                  |        3|          80|    0.00%|     26.67|      0.00|      0.00|
0.00|        38|      0/0
crafting_quest          |        1|           1|    0.00%|      1.00|      0.00|      0.00|
0.00|         1|      0/0
do_trans                |        1|         425|    0.00%|    425.00|      0.00|      0.00|
0.00|       425|      0/0
do_defensive_stance     |        1|        4848|    0.00%|   4848.00|      0.00|
[Page 12/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (12/15) ]
0.00|      0.00|      4848|      0/0
do_examine              |        2|          65|    0.00%|     32.50|      0.00|      0.00|
0.00|        40|      0/0
do_class                |        2|          27|    0.00%|     13.50|      0.00|      0.00|
0.00|        18|      0/0
do_quit                 |        1|      230922|    0.00%| 230922.00|      0.00|      0.00|
0.00|    230922|      0/0
do_users                |        1|          29|    0.00%|     29.00|      0.00|      0.00|
0.00|        29|      0/0
phantom                 |        1|           1|    0.00%|      1.00|      0.00|      0.00|
0.00|         1|      0/0
do_perfmon              |        0|           0|    0.00%|      0.00|      0.00|      0.00|
0.00|         0|      0/0
Cumulative game-loop telemetry
Event queue: calls=111280 callbacks=97114 created=26999 depth=78->71 max_before=274 max_after=270
Extractions: calls=111280 pending_before=1035 processed=1035 pending_after=0 max_processed=180
max_pending_before=180 max_pending_after=0
Catch-up: passes=343 budget_exhausted=1 requested_missed=2727 replayed_missed=2717
remaining_backlog=10 max_requested=41 max_remaining=10
Event callback registry: registered=22/512 report_limit=16 overflow_calls=0
Event callbacks (top 16 by total time)
Identity                            |    Calls|  Total usec|  Avg usec|  Max usec
-----------------------------------------------------------------------------------
Combat Round                        |     4903|    30931606|   6308.71|    623296
trig_wait_event                     |    29819|    12072821|    404.87|     21390
Mob Purge                           |       50|      157169|   3143.38|      4966
Check Occupied                      |    38702|       72544|      1.87|       100
Casting                             |     1226|       70225|     57.28|      1942
Encounter Region Reset              |      153|       49693|    324.79|       453
[Page 13/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (13/15) ]
Move Action Cooldown                |    21409|       36002|      1.68|        44
Spell Preparation                   |      173|       19709|    113.92|       306
Falling                             |      132|        4115|     31.17|        74
Crafting                            |       48|         742|     15.46|        57
Standard Action Cooldown            |      342|         730|      2.13|        13
Protocol                            |       19|         489|     25.74|        49
SoV Chain Lightning                 |        7|         164|     23.43|        35
SoV Ice Storm                       |        7|         147|     21.00|        26
Blur attack delay                   |       69|          42|      0.61|         1
RoL Drow Equipment Decay            |       22|          25|      1.14|         3
Memory Monitoring Dashboard
Window & Uptime:
  Elapsed Since Boot:       03h 06m 12s
  Elapsed Since Reset:      03h 05m 29s
Operating System Memory (/proc/self/status & rusage):
  Virtual Size (VmSize):     1595.67 MB (1633964 KB)
  Resident Set (VmRSS):      1505.18 MB (1541304 KB)  [Peak: 1505.18 MB]
  Anonymous RSS (RssAnon):   1484.12 MB (1519740 KB)  [98.6% of RSS]
  File-Backed RSS (RssFile):   21.06 MB (21564 KB)
  Shared Memory (RssShmem):     0.00 MB (0 KB)
  Data Segment (VmData):     1493.70 MB (1529552 KB)
  Swap Used (VmSwap):           0.00 MB (0 KB)
  Peak MaxRSS (rusage):      1507.75 MB (1543936 KB)
Heap Allocator (glibc mallinfo):
[Page 14/15]
[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (14/15) ]
  In-Use Heap (uordblks):    1054.82 MB (1080131 KB)
  Free in Arena (fordblks):     2.80 MB (2872 KB)
  Mmap Allocated (hblkhd):    414.27 MB (424212 KB)
  Total Arena (arena):       1057.62 MB (1083004 KB)
Memory Growth Analysis (Since Reset):
  RSS Net Change:             +75.32 MB (+77132 KB) [+415.8 KiB/min, +24.37 MiB/hr]
  Anonymous RSS Net Change:   +73.94 MB (+75716 KB) [+408.2 KiB/min, +23.92 MiB/hr]
  Heap In-Use Net Change:     +69.57 MB (+71240 KB) [+384.1 KiB/min, +22.50 MiB/hr]
  Status Assessment:        WARNING - Elevated growth rate (monitor closely)
Live Game Entity Inventory:
  Sockets / Descriptors:    10 connected (10 playing)
  Characters in World:      65903 total (10 PCs, 65893 Mobs) [+4756 total, +4756 Mobs]
  Objects in World:         53512 [+4001 since reset]
  Spell Affect Nodes:       67 across 32 characters [+51 nodes, +26 characters]
  NPC Followers:            4209 total (259 charmed) [+478 total, +43 charmed]
  Rooms & Zones:            91729 rooms across 764 zones
  Active Timed Events:      74 [-4 since reset]
  Pending Extractions:      0
Database queries since reset: 338189
[Page 15/15]
```
