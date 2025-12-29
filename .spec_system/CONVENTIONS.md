# CONVENTIONS.md

## LuminariMUD Coding Standards

This file adapts the Apex Spec conventions to the LuminariMUD ANSI C90/C89 codebase.

## Guiding Principles

- Optimize for readability over cleverness
- Code is written once, read many times
- Consistency beats personal preference
- If it can be automated, automate it
- When writing code: Make NO assumptions. Do not be lazy. Pattern match precisely. Do not skim when you need detailed info from documents. Validate systematically.

## Language Requirements

- **Standard:** ANSI C90/C89 with GNU extensions
- **Exception:** perfmon.cpp uses C++11
- **Compiler:** GCC or Clang with -Wall -Wextra
- **Format:** Use .clang-format configuration

### C90 Restrictions
- NO // comments (use /* */ only)
- NO declarations after statements
- NO variable-length arrays
- NO inline functions without __inline__ GNU extension

## Naming

### Functions and Variables
```c
/* lower_snake_case for functions and variables */
int calculate_damage(struct char_data *attacker);
int hit_points = 100;
```

### Macros and Constants
```c
/* UPPER_SNAKE_CASE for macros and constants */
#define MAX_LEVEL 30
#define GET_LEVEL(ch) ((ch)->player.level)
```

### Types
```c
/* lowercase with _data suffix for structs */
struct char_data { /* ... */ };
struct vessel_data { /* ... */ };
```

## Indentation and Formatting

- Two-space indentation
- Allman-style braces (opening brace on new line)
- Right-aligned pointers (char *ptr, not char* ptr)
- 100-column limit

```c
void example_function(int value)
{
  if (value > 0)
  {
    char *buffer = NULL;
    perform_action(buffer, value);
  }
}
```

## Comments

- Use /* */ block comments only (no //)
- Explain *why*, not *what*
- Delete commented-out code--that's what git is for
- Update or remove comments when code changes

### Function Documentation
```c
/**
 * Brief description of function purpose.
 *
 * Detailed explanation if needed, including edge cases
 * and important behavioral notes.
 *
 * @param ch The character performing the action
 * @param victim The target of the action
 * @return TRUE if successful, FALSE otherwise
 */
int perform_action(struct char_data *ch, struct char_data *victim)
{
  /* Implementation */
}
```

## Error Handling

- **NULL checks:** Always check pointers before dereferencing
- **Memory management:** Free all allocated memory; use Valgrind for verification
- **Buffer safety:** Use safe string functions (snprintf, not sprintf)
- **No hardcoded values:** Use #defines or configuration files
- **Log errors appropriately:** Don't silently fail

```c
if (ptr == NULL)
{
  log("SYSERR: Unexpected NULL pointer in function_name!");
  return;
}
```

## Memory Management

```c
/* Use CREATE macro for allocation */
CREATE(ptr, struct vessel_data, 1);

/* Always check allocation success */
if (!ptr)
{
  log("SYSERR: Out of memory!");
  return;
}

/* Free memory when done */
free(ptr);
ptr = NULL;
```

## Common Macros

```c
/* Character attribute getters/setters */
GET_LEVEL(ch)           /* Character level */
GET_HIT(ch)             /* Current HP */
GET_MAX_HIT(ch)         /* Maximum HP */
GET_CLASS(ch)           /* Character class */
GET_RACE(ch)            /* Character race */

/* Utility macros */
CREATE(ptr, type, num)  /* Allocate memory */
REMOVE_BIT(var, bit)    /* Clear bit flag */
SET_BIT(var, bit)       /* Set bit flag */
IS_NPC(ch)              /* Check if character is NPC */
```

## Testing

- Test behavior, not implementation
- A test's name should describe the scenario and expectation
- If it's hard to test, the design might need rethinking
- Flaky tests get fixed or deleted--never ignored

### CuTest Pattern
```c
void test_my_function(CuTest *tc)
{
  /* Arrange */
  int result;

  /* Act */
  result = my_function(10);

  /* Assert */
  CuAssertIntEquals(tc, 20, result);
}
```

## Git and Version Control

- Commit messages: imperative mood, concise ("Add user validation" not "Added some validation stuff")
- One logical change per commit
- Branch names: type/short-description (e.g., feat/vessel-docking, fix/room-allocation)
- Keep commits atomic enough to revert safely
- NEVER include AI attribution in commits

## Configuration Files

- **Never commit:** campaign.h, mud_options.h, vnums.h (environment-specific)
- **Use .example.h files:** Provide templates for environment-specific configs
- **Secrets:** Never commit credentials or tokens

## Database Operations

```c
/* Always check MySQL connection */
if (!mysql_connection)
{
  log("SYSERR: No MySQL connection!");
  return;
}

/* Use prepared statements when possible */
/* Free result sets after use */
```

## Local Dev Tools

| Category | Tool | Config |
|----------|------|--------|
| Formatter | clang-format | .clang-format |
| Linter | clang-tidy | .clang-tidy |
| Type Safety | GCC/Clang -Wall -Wextra | Makefile/CMakeLists.txt |
| Testing | CuTest | unittests/CuTest/Makefile |
| Git Hooks | pre-commit | .pre-commit-config.yaml |

## CI/CD

| Bundle | Status | Workflow |
|--------|--------|----------|
| Code Quality | configured | .github/workflows/quality.yml |
| Build & Test | not configured | - |
| Security | not configured | - |
| Integration | not configured | - |
| Operations | partial | .github/workflows/pages.yml (docs only) |

## When In Doubt

- Ask
- Leave it better than you found it
- Ship, learn, iterate
- Check existing code patterns in the codebase
- Run tests and Valgrind before committing
