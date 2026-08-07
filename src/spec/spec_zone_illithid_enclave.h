/**
 * @file spec/spec_zone_illithid_enclave.h
 * Public API for Illithid Enclave zone procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_ILLITHID_ENCLAVE_H
#define LUMINARI_SPEC_ZONE_ILLITHID_ENCLAVE_H

struct char_data;

int illithid_gguard(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_ILLITHID_ENCLAVE_H */
