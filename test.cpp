#include "catch.hpp"

#include "src/modutils.h"

TEST_CASE("modutils", "[machine]")
{
	auto x = getTypeAndBase("/blaj/mdat.gurgle%tjosan");
	REQUIRE(x == std::make_tuple("mdat", "gurgle%tjosan"));

	x = getTypeAndBase("/blaj/skurk.mannen.x.mod");
	REQUIRE(x == std::make_tuple("mod", "skurk.mannen.x"));

	x = getTypeAndBase("/blaj/mod/mdat/hejsan hoppsan.whatever");
	REQUIRE(x == std::make_tuple("whatever", "hejsan hoppsan"));

	REQUIRE(getBaseName("/asda/das/test.mod") == "test.mod");
	REQUIRE(getTypeFromName("gurgle.format") == "format");
	REQUIRE(getTypeFromName("mdat.gurgle") == "mdat");
	REQUIRE(getTypeFromName("mdat.gurgle") == "mdat");
	REQUIRE(getTypeFromName("ftp%3a%2f%2fftp.modland.com%2fpub%2fmodules%2fSunTronic%2fTSM%2fmsx-intro.sun") == "sun");
	REQUIRE(getTypeFromName("ftp%3a%2f%2fftp.modland.com%2fpub%2fmodules%2fTFMX%2fChris Huelsbeck%2fmdat.apidya (level 3)") == "mdat");
}
