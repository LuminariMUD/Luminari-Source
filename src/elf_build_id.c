/**************************************************************************
 *  File: elf_build_id.c                               Part of LuminariMUD *
 *  Usage: Identify the executable image that is actually running.         *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

/* dl_iterate_phdr() and struct dl_phdr_info are GNU extensions.  This file is
 * kept separate so the feature macro does not affect any other translation
 * unit. */
#define _GNU_SOURCE 1

#include <stdio.h>
#include <string.h>

#if defined(__linux__) && defined(__GLIBC__)
#define LUMINARI_HAVE_SELF_BUILD_ID 1
#include <link.h>
#include <elf.h>
#endif

#include "elf_build_id.h"

#ifdef LUMINARI_HAVE_SELF_BUILD_ID

/* Hex form of this executable's GNU build ID, filled in on each request. */
static char self_elf_build_id[129] = "";

/* dl_iterate_phdr() callback.  The main executable is always reported first,
 * so we inspect only the first object and then stop the walk. */
static int scan_self_build_id(struct dl_phdr_info *info, size_t size, void *data)
{
  size_t i = 0;

  (void)size;
  (void)data;

  for (i = 0; i < info->dlpi_phnum; i++)
  {
    const char *note = NULL;
    const char *note_end = NULL;

    if (info->dlpi_phdr[i].p_type != PT_NOTE)
      continue;

    note = (const char *)(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
    note_end = note + info->dlpi_phdr[i].p_memsz;

    while (note + sizeof(ElfW(Nhdr)) <= note_end)
    {
      const ElfW(Nhdr) *nhdr = (const ElfW(Nhdr) *)note;
      const char *name = note + sizeof(ElfW(Nhdr));
      const char *desc = name + ((nhdr->n_namesz + 3) & ~(ElfW(Word))3);

      if (desc > note_end || (size_t)(note_end - desc) < nhdr->n_descsz)
        break;

      if (nhdr->n_type == NT_GNU_BUILD_ID && nhdr->n_namesz == 4 && memcmp(name, "GNU", 4) == 0 &&
          nhdr->n_descsz > 0 && nhdr->n_descsz <= (sizeof(self_elf_build_id) - 1) / 2)
      {
        ElfW(Word) byte = 0;

        for (byte = 0; byte < nhdr->n_descsz; byte++)
          snprintf(self_elf_build_id + (byte * 2), 3, "%02x", (unsigned char)desc[byte]);
        return 1;
      }

      note = desc + ((nhdr->n_descsz + 3) & ~(ElfW(Word))3);
    }
  }

  return 1; /* Only the main object is of interest. */
}

#endif /* LUMINARI_HAVE_SELF_BUILD_ID */

/* Return the running executable's GNU build ID as lowercase hex, or NULL when
 * it cannot be read.  Copyover replaces the process with execl(), which keeps
 * the previous release's environment, so an inherited LUMINARI_ELF_BUILD_ID
 * describes the binary we came from rather than the one now running. */
const char *get_self_elf_build_id(void)
{
#ifdef LUMINARI_HAVE_SELF_BUILD_ID
  self_elf_build_id[0] = '\0';
  dl_iterate_phdr(scan_self_build_id, NULL);
  if (self_elf_build_id[0] != '\0')
    return self_elf_build_id;
#endif
  return NULL;
}
