#include "gason.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

void gason_print_error(const char *filename, JsonParseStatus status, char *endptr, char *source, size_t size)
{
	const int INDENT = 4;
	const char *status2str[] = {
		"JSON_PARSE_OK",
		"JSON_PARSE_BAD_NUMBER",
		"JSON_PARSE_BAD_STRING",
		"JSON_PARSE_BAD_IDENTIFIER",
		"JSON_PARSE_STACK_OVERFLOW",
		"JSON_PARSE_STACK_UNDERFLOW",
		"JSON_PARSE_MISMATCH_BRACKET",
		"JSON_PARSE_UNEXPECTED_CHARACTER",
		"JSON_PARSE_UNQUOTED_KEY",
		"JSON_PARSE_BREAKING_BAD"
	};
	char *s = endptr;
	while (s != source && *s != '\n') --s;
	if (s != endptr && s != source) ++s;
	int line = 0;
	for (char *i = s; i != source; --i)
	{
		if (*i == '\n')
		{
			++line;
		}
	}
	int column = (int)(endptr - s);
	fprintf(stderr, "%s:%d:%d: error %s\n", filename, line + 1, column + 1, status2str[status]);
	while (s != source + size && *s != '\n')
	{
		int c = *s++;
		switch (c)
		{
			case '\b': fprintf(stderr, "\\b"); column += 1; break;
			case '\f': fprintf(stderr, "\\f"); column += 1; break;
			case '\n': fprintf(stderr, "\\n"); column += 1; break;
			case '\r': fprintf(stderr, "\\r"); column += 1; break;
			case '\t': fprintf(stderr, "%*s", INDENT, ""); column += INDENT - 1; break;
			case 0: fprintf(stderr, "\""); break;
			default: fprintf(stderr, "%c", c);
		}
	}
	fprintf(stderr, "\n%*s\n", column + 1, "^");
}

const char* jsonTagToString(JsonTag tag)
{
	switch(tag)
	{
		case JSON_TAG_NUMBER: return "integer";
		case JSON_TAG_BOOL: return "bool";
		case JSON_TAG_STRING: return "string";
		case JSON_TAG_ARRAY: return "array";
		case JSON_TAG_OBJECT: return "object";
		case JSON_TAG_NULL: return "null";
		default: abort();
	}
}
