/* Pet persistence identity, restore validation, and failure-injection checks. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/mudlim.h"

/* A saved pet belongs to the pfile identity, not to the reusable owner name.
 * Both halves of the binding must be reported so a recycled ID cannot adopt an
 * earlier character's pets. */
void Test_pet_owner_binding_reports_the_stable_pfile_identity(CuTest *tc)
{
  struct char_data owner;
  long int owner_id;
  long long owner_created;

  clear_char(&owner);
  GET_IDNUM(&owner) = 4711;
  owner.player.time.birth = (time_t)1600000000;

  pet_owner_binding(&owner, &owner_id, &owner_created);
  CuAssertIntEquals(tc, 4711, (int)owner_id);
  CuAssertTrue(tc, owner_created == 1600000000LL);

  /* A missing owner yields an unbound pair rather than a partial identity. */
  pet_owner_binding(NULL, &owner_id, &owner_created);
  CuAssertIntEquals(tc, 0, (int)owner_id);
  CuAssertTrue(tc, owner_created == 0LL);
}

/* Restore check: a saved object graph is published only when every nested
 * container is closed and no worn slot is claimed twice. */
void Test_pet_object_graph_rejects_malformed_restores(CuTest *tc)
{
  struct char_data pet;
  struct obj_data container;
  struct obj_data content;
  struct obj_data worn;
  obj_save_data records[2] = {{0}};

  clear_char(&pet);
  memset(&container, 0, sizeof(container));
  memset(&content, 0, sizeof(content));
  memset(&worn, 0, sizeof(worn));
  GET_OBJ_TYPE(&container) = ITEM_CONTAINER;
  GET_OBJ_TYPE(&content) = ITEM_OTHER;
  GET_OBJ_TYPE(&worn) = ITEM_OTHER;

  /* Contents are written before their parent with a negative depth. */
  records[0].obj = &content;
  records[0].locate = -1;
  records[0].next = &records[1];
  records[1].obj = &container;
  records[1].locate = 1;
  records[1].next = NULL;
  CuAssertTrue(tc, pet_object_graph_valid_for_test(&pet, records));

  /* A pending content with no container to close it is malformed. */
  records[1].obj = NULL;
  records[0].next = NULL;
  CuAssertTrue(tc, !pet_object_graph_valid_for_test(&pet, records));

  /* Two records cannot claim the same worn slot. */
  records[0].obj = &worn;
  records[0].locate = 2;
  records[0].next = &records[1];
  records[1].obj = &container;
  records[1].locate = 2;
  records[1].next = NULL;
  CuAssertTrue(tc, !pet_object_graph_valid_for_test(&pet, records));

  /* A location outside the wear table is refused outright. */
  records[0].obj = &worn;
  records[0].locate = NUM_WEARS + 1;
  records[0].next = NULL;
  CuAssertTrue(tc, !pet_object_graph_valid_for_test(&pet, records));
}
