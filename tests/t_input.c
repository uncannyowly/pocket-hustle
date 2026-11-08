#include "check.h"
#include "tests.h"
#include "ui/input.h"

static void test_edge_fires_once(void)
{
	Repeat r = {0};
	CHECK_EQ(input_step(&r, 0, 0), 0);
	CHECK_EQ(input_step(&r, PAD_BIT(PAD_A), 10), PAD_BIT(PAD_A));
	CHECK_EQ(input_step(&r, PAD_BIT(PAD_A), 20), 0);
	CHECK_EQ(input_step(&r, PAD_BIT(PAD_A) | PAD_BIT(PAD_B), 30), PAD_BIT(PAD_B));
}

static void test_dpad_repeats(void)
{
	Repeat r = {0};
	uint32_t up = PAD_BIT(PAD_UP);
	CHECK_EQ(input_step(&r, up, 0), up);
	CHECK_EQ(input_step(&r, up, 299), 0);
	CHECK_EQ(input_step(&r, up, 300), up);
	CHECK_EQ(input_step(&r, up, 379), 0);
	CHECK_EQ(input_step(&r, up, 380), up);
	CHECK_EQ(input_step(&r, up, 460), up);
}

static void test_buttons_never_repeat(void)
{
	static const int btn[] = {PAD_A, PAD_B, PAD_L1, PAD_R1, PAD_START};
	size_t i;
	for (i = 0; i < sizeof btn / sizeof btn[0]; i++) {
		Repeat r = {0};
		uint32_t t, n = 0;
		for (t = 0; t <= 2000; t += 10)
			if (input_step(&r, PAD_BIT(btn[i]), t))
				n++;
		CHECK_EQ(n, 1);
	}
}

static void test_release_resets(void)
{
	Repeat r = {0};
	CHECK_EQ(input_step(&r, PAD_BIT(PAD_DOWN), 0), PAD_BIT(PAD_DOWN));
	CHECK_EQ(input_step(&r, 0, 100), 0);
	CHECK_EQ(input_step(&r, PAD_BIT(PAD_DOWN), 110), PAD_BIT(PAD_DOWN));
	CHECK_EQ(input_step(&r, PAD_BIT(PAD_DOWN), 300), 0);
	CHECK_EQ(input_step(&r, PAD_BIT(PAD_DOWN), 410), PAD_BIT(PAD_DOWN));
}

static void test_token(void)
{
	CHECK_EQ(input_token("U"), PAD_UP);
	CHECK_EQ(input_token("D"), PAD_DOWN);
	CHECK_EQ(input_token("L"), PAD_LEFT);
	CHECK_EQ(input_token("R"), PAD_RIGHT);
	CHECK_EQ(input_token("A"), PAD_A);
	CHECK_EQ(input_token("B"), PAD_B);
	CHECK_EQ(input_token("X"), PAD_X);
	CHECK_EQ(input_token("Y"), PAD_Y);
	CHECK_EQ(input_token("L1"), PAD_L1);
	CHECK_EQ(input_token("R1"), PAD_R1);
	CHECK_EQ(input_token("SEL"), PAD_SELECT);
	CHECK_EQ(input_token("START"), PAD_START);
	CHECK_EQ(input_token("a"), -1);
	CHECK_EQ(input_token("L2"), -1);
	CHECK_EQ(input_token(""), -1);
	CHECK_EQ(input_token("UP"), -1);
}

void tests_input(void)
{
	test_token();
	test_edge_fires_once();
	test_dpad_repeats();
	test_buttons_never_repeat();
	test_release_resets();
}
