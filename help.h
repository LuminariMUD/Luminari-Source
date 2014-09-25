/* 
 * File:   help.h
 * Author: Ornir (Jamie McLaughlin)
 *
 * Created on 25. september 2014, 10:26
 */

#ifndef HELP_H
#define	HELP_H

struct help_entry_list {
  char *keyword;
  char *alternate_keywords;
  char *entry;
  int  min_level;
  char *last_updated;

  struct help_entry_list *next;
};

struct help_entry_list * search_help(const char *argument, int level);
void perform_help(struct descriptor_data *d, char *argument);

ACMD(do_help);

#endif	/* HELP_H */

