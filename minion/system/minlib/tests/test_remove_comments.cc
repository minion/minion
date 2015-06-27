#include "minlib/string_ops.hpp"

int main(void)
{
	D_ASSERT(removeComments("") == "");
	D_ASSERT(removeComments("/") == "/");
	D_ASSERT(removeComments("//") == "");
	D_ASSERT(removeComments("a//b") == "a");
	D_ASSERT(removeComments("a//b\n") == "a\n");
	D_ASSERT(removeComments("a//b\n\n") == "a\n\n");
	D_ASSERT(removeComments("xyz") == "xyz");
	
}