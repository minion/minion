#include "minlib/minlib.hpp"
#include "minlib/variant.hpp"

using boost::variant;

int main(void)
{
	variant<int, double, std::string> v;
	v = 1;
	D_ASSERT(variant_istype<int>(v));
	D_ASSERT(!variant_istype<double>(v));
	D_ASSERT(!variant_istype<std::string>(v));
	D_ASSERT(!variant_istype<long long>(v));

	v = std::string("cat");

	D_ASSERT(!variant_istype<int>(v));
	D_ASSERT(!variant_istype<double>(v));
	D_ASSERT(variant_istype<std::string>(v));
	D_ASSERT(!variant_istype<long long>(v));
}
