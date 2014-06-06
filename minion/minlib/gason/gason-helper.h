#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "gason.h"

const char* jsonTagToString(JsonTag tag);
void gason_print_error(const char *filename, JsonParseStatus status, char *endptr, char *source, size_t size);
