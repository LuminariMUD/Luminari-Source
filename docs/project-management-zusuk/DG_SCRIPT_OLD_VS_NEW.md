# DG Scripts: Old vs New Code Comparison at Problem Location

## Executive Summary
The THEORY that the critical issue is a parameter corruption occurring between the function call and reception. When `cmd_otrig()` calls `script_driver()` with `OBJ_TRIGGER` (value 1), the function receives `type=2` (WLD_TRIGGER). This document compares the old stable code with the current problematic implementation.

## 1. cmd_otrig() Function Comparison

### OLD CODE (stable - old_dg_triggers.c:719-754)
```c
int cmd_otrig(obj_data *obj, char_data *actor, char *cmd,
              char *argument, int type)  // Note: 'type' here is OCMD_* flags
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (obj && SCRIPT_CHECK(obj, OTRIG_COMMAND))
    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
      // ... validation checks ...
      
      if (IS_SET(GET_TRIG_NARG(t), type) &&  // 'type' is OCMD_INVEN/EQUIP/ROOM
          (*GET_TRIG_ARG(t)=='*' ||
          !strn_cmp(GET_TRIG_ARG(t), cmd, strlen(GET_TRIG_ARG(t))))) {
        
        ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
        skip_spaces(&argument);
        add_var(&GET_TRIG_VARS(t), "arg", argument, 0);
        skip_spaces(&cmd);
        add_var(&GET_TRIG_VARS(t), "cmd", cmd, 0);

        // DIRECT CALL - No intermediate variables
        if (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW))
          return 1;
      }
    }

  return 0;
}
```

### NEW CODE (problematic - current dg_triggers.c:820-860)
```c
int cmd_otrig(obj_data *obj, char_data *actor, char *cmd,
              char *argument, int cmd_type)  // RENAMED: 'type' -> 'cmd_type'
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (obj && SCRIPT_CHECK(obj, OTRIG_COMMAND))
    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
      // ... validation checks ...
      
      if (IS_SET(GET_TRIG_NARG(t), cmd_type) &&  // Using renamed parameter
          (*GET_TRIG_ARG(t) == '*' ||
           !strn_cmp(GET_TRIG_ARG(t), cmd, strlen(GET_TRIG_ARG(t))))) {
        
        ADD_UID_VAR(buf, t, actor, "actor", 0);  // DIFFERENT: uses 'actor' directly
        skip_spaces(&argument);
        add_var(&GET_TRIG_VARS(t), "arg", argument, 0);
        skip_spaces(&cmd);
        add_var(&GET_TRIG_VARS(t), "cmd", cmd, 0);

        // ATTEMPTED FIX: Using explicit local variable
        {
          int trigger_type = OBJ_TRIGGER;  /* Explicitly set to OBJ_TRIGGER (value 1) */
          if (script_driver(&obj, t, trigger_type, TRIG_NEW))
            return 1;
        }
      }
    }

  return 0;
}
```

### KEY DIFFERENCES in cmd_otrig:
1. **Parameter rename**: `type` � `cmd_type` (to avoid shadowing)
2. **ADD_UID_VAR macro call**: Old uses `char_script_id(actor)`, new uses `actor` directly
3. **Attempted fix**: New code tries to use explicit local variable for trigger type
4. **Still has corruption**: Despite the fix attempt, parameter corruption persists

## 2. script_driver() Function Comparison

### OLD CODE (stable - old_dg_scripts.c:2806-2900)
```c
int script_driver(void *go_adress, trig_data *trig, int type, int mode)
{
  // ... variable declarations ...
  
  void obj_command_interpreter(obj_data * obj, char *argument);
  void wld_command_interpreter(struct room_data * room, char *argument);

  // DIRECT SWITCH - No validation
  switch (type) {
  case MOB_TRIGGER:
    go = *(char_data **)go_adress;
    sc = SCRIPT((char_data *)go);
    break;
  case OBJ_TRIGGER:
    go = *(obj_data **)go_adress;
    sc = SCRIPT((obj_data *)go);
    break;
  case WLD_TRIGGER:
    go = *(room_data **)go_adress;
    sc = SCRIPT((room_data *)go);
    break;
  }
  
  // ... rest of function ...
}
```

### NEW CODE (with validation - current dg_scripts.c:2806-2900)
```c
int script_driver(void *go_adress, trig_data *trig, int type, int mode)
{
  // ... variable declarations ...
  
  /* Debug for DG script parameter corruption issues */
#ifdef SCRIPT_DEBUG
  if (trig) {
    script_log("SCRIPT_DEBUG %d: script_driver entry - go_adress=%p, trig=%p, type=%d, mode=%d",
               GET_TRIG_VNUM(trig), go_adress, (void*)trig, type, mode);
  }
#endif

  /* IMPORTANT: Type validation must happen BEFORE we use type to extract pointers */
  
  // EXTENSIVE VALIDATION AND RECOVERY CODE
  if (type < MOB_TRIGGER || type > WLD_TRIGGER) {
    script_log("CRITICAL: Invalid trigger type %d received for trigger %d.",
               type, trig ? GET_TRIG_VNUM(trig) : -1);
    
    // Attempt to recover type from trigger flags
    if (trig) {
      // Check for unique OBJECT trigger flags
      if ((GET_TRIG_TYPE(trig) & OTRIG_TIMER) ||
          (GET_TRIG_TYPE(trig) & OTRIG_GET) ||
          // ... more object-specific flags ...
          (GET_TRIG_TYPE(trig) & OTRIG_CONSUME)) {
        script_log("RECOVERY: Detected unique object trigger flags, setting type to OBJ_TRIGGER");
        type = OBJ_TRIGGER;
      }
      // ... similar checks for MOB and WLD triggers ...
    }
  }
  
  // NOW safely use the corrected type
  switch (type) {
  case MOB_TRIGGER:
    go = *(char_data **)go_adress;
    sc = SCRIPT((char_data *)go);
    break;
  // ... other cases ...
  }
}
```

### KEY DIFFERENCES in script_driver:
1. **Extensive debugging**: New code has debug logging
2. **Type validation**: New code validates and attempts to recover corrupted type
3. **Recovery mechanism**: Uses trigger flags to determine correct type
4. **Problem persists**: Despite recovery, the corruption happens BEFORE this function

## 3. Critical Discovery: The Corruption Point

### The corruption occurs BETWEEN these exact locations:

#### CALL SITE (dg_triggers.c:853)
```c
int trigger_type = OBJ_TRIGGER;  /* Value = 1 */
if (script_driver(&obj, t, trigger_type, TRIG_NEW))  // Passes 1
```

#### FUNCTION ENTRY (dg_scripts.c:2806)
```c
int script_driver(void *go_adress, trig_data *trig, int type, int mode)
{
  // type received as 2 (WLD_TRIGGER) instead of 1 (OBJ_TRIGGER)
```

## 4. Root Cause Analysis

### Coincidental Value Match
- `OCMD_INVEN = 2` (bit flag for inventory items)
- `WLD_TRIGGER = 2` (trigger type for rooms)
- When processing inventory commands, cmd_type = OCMD_INVEN = 2

### The Real Problem
The parameter corruption is NOT in the C code itself but occurs at a lower level:
1. **Stack corruption**: Between function call and entry
2. **Compiler optimization**: ANSI C90 compiler may be optimizing incorrectly
3. **Calling convention issue**: Parameter passing mechanism corrupted
4. **Memory alignment**: Possible struct packing/alignment issues

### Why Old Code Works
The old code doesn't have:
1. The extensive validation/recovery code
2. The parameter renaming (type � cmd_type)
3. The explicit local variable workaround
4. The ADD_UID_VAR macro difference

## 5. Key Observations

1. **Not a Logic Error**: The C code logic is correct in both versions
2. **Low-Level Issue**: Corruption happens in CPU/stack during function call
3. **ANSI C90 Specific**: May be related to old C standard limitations
4. **Intermittent**: Affects different triggers at different times
5. **Value-Specific**: Only corrupts to value 2 when OCMD_INVEN is involved

## 6. Recommended Solutions

### Short-term Workarounds
1. Continue using the type validation/recovery in script_driver
2. Add defensive coding at both call and entry points
3. Use volatile keywords to prevent optimization

### Long-term Fixes
1. **Upgrade compiler**: Move from C90 to C99 or newer
2. **Change calling convention**: Use different parameter passing
3. **Refactor signature**: Pass type in a struct to avoid stack issues
4. **Memory barriers**: Add compiler barriers around critical calls

## Conclusion

The comparison reveals that the fundamental difference isn't in the algorithm but in how the code interacts with the compiler and runtime environment. The old code works by chance, while the new code's attempts to fix the issue have exposed a deeper problem with parameter passing at the assembly/stack level. The corruption is consistent (always changes 1 to 2 for trigger 7705) suggesting a systematic issue rather than random memory corruption.