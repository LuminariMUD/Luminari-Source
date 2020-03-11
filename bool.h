#pragma once

/* C99 has bool type */
#include <stdbool.h>

#if 0
#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char bool; /**< Technically 1 signed byte; vals should only = TRUE or FALSE. */
#endif

#if !defined(FALSE)
/** Just in case FALSE is not defined. */
#define FALSE 0
#endif

#if !defined(TRUE)
/** Just in case TRUE is not defined. */
#define TRUE  (!FALSE)
#endif
#endif

#if !defined(FALSE)
/** Just in case FALSE is not defined. */
#define FALSE false
#endif

#if !defined(TRUE)
/** Just in case TRUE is not defined. */
#define TRUE true
#endif
