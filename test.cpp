#include "catch.hpp"

#include "src/modutils.h"

TEST_CASE("modutils", "[machine]")
{
    auto x = getTypeFromName("test.mod");
    puts(x.c_str());
}
