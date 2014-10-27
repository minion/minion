#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string>
#include <vector>

#include "gason.h"

const char* jsonTagToString(JsonTag tag);
void gason_print_error(const char *filename, JsonParseStatus status, char *endptr, char *source, size_t size);

JsonValue jsonGetKey(JsonValue js, std::string key);

std::vector<int> jsonReadIntArray(JsonValue js);
std::vector<std::string> jsonReadStringArray(JsonValue js);
