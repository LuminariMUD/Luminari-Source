#ifndef LUMINARI_NET_MSDP_JSON_H
#define LUMINARI_NET_MSDP_JSON_H

#include <stddef.h>

#include "protocol.h"

protocol_error_t msdp_json_validate_name(const char *name);
protocol_error_t msdp_json_validate_scalar(const char *value);
protocol_error_t msdp_json_validate_structured(const char *value);

protocol_error_t msdp_json_build_string_frame(char *frame, size_t capacity, const char *variable,
                                              const char *value, size_t *frame_length);
protocol_error_t msdp_json_build_number_frame(char *frame, size_t capacity, const char *variable,
                                              int value, size_t *frame_length);
protocol_error_t msdp_json_build_list_frame(char *frame, size_t capacity, const char *variable,
                                            const char *values, size_t *frame_length);

#endif /* LUMINARI_NET_MSDP_JSON_H */
