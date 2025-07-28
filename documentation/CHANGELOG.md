# CHANGELOG

## 2025-01-28
### Fixed
- Fixed `put all <container>` command not recognizing "all" keyword properly
  - Added missing `find_all_dots()` parsing in `do_put()` function in act.item.c:2009
  - Command was treating "all" as an object name instead of a keyword
  - Now correctly puts all items from inventory into the specified container

