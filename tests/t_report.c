#include "check.h"
#include "tests.h"
#include "report.h"

static void test_compare_band(void)
{
	CHECK_EQ(compare_value(60, 100, 0), V_OK);
	CHECK_EQ(compare_value(59, 100, 0), V_LOW);
	CHECK_EQ(compare_value(167, 100, 0), V_OK);
	CHECK_EQ(compare_value(168, 100, 0), V_HIGH);
}

static void test_compare_too_close(void)
{
	CHECK_EQ(compare_value(104, 100, 1), V_TOO_CLOSE);
	CHECK_EQ(compare_value(106, 100, 1), V_OK);
	CHECK_EQ(compare_value(104, 100, 0), V_OK);
	CHECK_EQ(compare_value(96, 100, 1), V_TOO_CLOSE);
}

static void test_target_names(void)
{
	CHECK(target_matches_good("Weed", "Weed"));
	CHECK(target_matches_good("Mushrooms", "Shrooms"));
	CHECK(target_matches_good("Ecstasy", "Molly"));
	CHECK(target_matches_good("Special K", "Ketamine"));
	CHECK(!target_matches_good("Crack", "Cocaine"));
}

void tests_report(void)
{
	test_compare_band();
	test_compare_too_close();
	test_target_names();
}
