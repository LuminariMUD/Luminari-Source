/**************************************************************************
 *  File: elf_build_id.h                               Part of LuminariMUD *
 *  Usage: Identify the executable image that is actually running.         *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#ifndef ELF_BUILD_ID_H
#define ELF_BUILD_ID_H

/* Lowercase hex GNU build ID of the running executable, or NULL if unreadable.
 * The returned pointer is owned by the module and is valid until the next call. */
const char *get_self_elf_build_id(void);

#endif /* ELF_BUILD_ID_H */
