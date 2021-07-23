#include "unity.h"
#include "bit_vector.h"

char const *const BIT_STR = "01010101_01010101_01010101_01010101_01010101_01010101_01010101_01010101";

BitVector *bv;

void setUp(void) {}

void tearDown(void) {}

void test_rank_one(void)
{
    size_t rank = rank_one(bv, 0);
    size_t expected = 0;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_one(bv, 40);
    expected = 20;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_one(bv, 64);
    expected = 32;
    TEST_ASSERT_EQUAL(expected, rank);
}

void test_rank_zero(void)
{
    size_t rank = rank_zero(bv, 0);
    size_t expected = 0;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_zero(bv, 40);
    expected = 20;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_zero(bv, 64);
    expected = 32;
    TEST_ASSERT_EQUAL(expected, rank);
}

void test_select_one_long_block(void)
{
    BitVector *bv = construct_bit_vector("01");

    size_t index = select_one(bv, 0);
    size_t expected = 1;
    TEST_ASSERT_EQUAL(expected, index);

    destruct_bit_vector(bv);
}

void test_select_one_short_block(void)
{
    size_t index = select_one(bv, 0);
    size_t expected = 1;
    TEST_ASSERT_EQUAL(expected, index);

    index = select_one(bv, 20);
    expected = 41;
    TEST_ASSERT_EQUAL(expected, index);

    index = select_one(bv, 31);
    expected = 63;
    TEST_ASSERT_EQUAL(expected, index);
}

void test_select_zero_long_block(void)
{
    BitVector *bv = construct_bit_vector("01");

    size_t index = select_zero(bv, 0);
    size_t expected = 0;
    TEST_ASSERT_EQUAL(expected, index);

    destruct_bit_vector(bv);
}

void test_select_zero_short_block(void)
{
    size_t index = select_zero(bv, 0);
    size_t expected = 0;
    TEST_ASSERT_EQUAL(expected, index);

    index = select_zero(bv, 20);
    expected = 40;
    TEST_ASSERT_EQUAL(expected, index);

    index = select_zero(bv, 31);
    expected = 62;
    TEST_ASSERT_EQUAL(expected, index);
}

int main(void)
{
    UNITY_BEGIN();
    bv = construct_bit_vector(BIT_STR);
    RUN_TEST(test_rank_one);
    RUN_TEST(test_rank_zero);
    RUN_TEST(test_select_one_long_block);
    RUN_TEST(test_select_one_short_block);
    RUN_TEST(test_select_zero_long_block);
    RUN_TEST(test_select_zero_short_block);
    destruct_bit_vector(bv);
    return UNITY_END();
}
