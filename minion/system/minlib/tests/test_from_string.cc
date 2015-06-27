#include "minlib/minlib.hpp"

int main(void)
{
	D_ASSERT(fromstring<int>("1") == 1);
	D_ASSERT(fromstring<int>("-1") == -1);
	D_ASSERT(fromstring<int>(" 6 ") == 6);
	D_ASSERT(fromstring<int>("42") == 42);

	D_ASSERT(is_fromstring<int>("1"));
	D_ASSERT(is_fromstring<int>("-1"));
	D_ASSERT(is_fromstring<int>(" 6 "));
	D_ASSERT(is_fromstring<int>("42"));

	bool caught = false;

	try
	{ fromstring<int>("1x"); }
	catch(...)
	{ caught = true; }

	D_ASSERT(caught);
	D_ASSERT(!is_fromstring<int>("1x"));

	caught = false;

	try
	{ fromstring<int>("x0"); }
	catch(...)
	{ caught = true; }

	D_ASSERT(caught);
	D_ASSERT(!is_fromstring<int>("x0"));
	caught = false;

	try
	{ fromstring<int>("!"); }
	catch(...)
	{ caught = true; }

	D_ASSERT(caught);
	D_ASSERT(!is_fromstring<int>("!"));

}
