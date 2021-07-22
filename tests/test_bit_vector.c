#include "unity.h"
#include "bit_vector.h"

char const * const BIT_STR = "01010101_01010101_01010101_01010101_01010101_01010101_01010101_01010101";

void setUp(void) {}
void tearDown(void) {}

void test_construction_destruction(void) {
    BitVector *bv = construct_bit_vector(BIT_STR);
    // print_bit_vector(bv);
    destruct_bit_vector(bv);
}

void test_rank_1(void) {
    BitVector *bv = construct_bit_vector(BIT_STR);

    size_t rank = rank_1(bv, 0);
    size_t expected = 0;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_1(bv, 31);
    expected = 15;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_1(bv, 64);
    expected = 32;
    TEST_ASSERT_EQUAL(expected, rank);

    destruct_bit_vector(bv);
}

void test_rank_0(void) {
    BitVector *bv = construct_bit_vector(BIT_STR);

    size_t rank = rank_0(bv, 0);
    size_t expected = 0;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_0(bv, 31);
    expected = 16;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_0(bv, 64);
    expected = 32;
    TEST_ASSERT_EQUAL(expected, rank);

    destruct_bit_vector(bv);
}

void test_select_1_long_block(void) {
    BitVector *bv = construct_bit_vector("01");

    size_t index = select_1(bv, 0);
    size_t expected = 1;
    TEST_ASSERT_EQUAL(expected, index);

    destruct_bit_vector(bv);
}

void test_select_1_short_block(void) {
    BitVector *bv = construct_bit_vector(BIT_STR);

    // print_bit_vector(bv);
    size_t index = select_1(bv, 0);
    size_t expected = 1;
    TEST_ASSERT_EQUAL(expected, index);

    index = select_1(bv, 16);
    expected = 33;
    TEST_ASSERT_EQUAL(expected, index);

    index = select_1(bv, 31);
    expected = 63;
    TEST_ASSERT_EQUAL(expected, index);

    destruct_bit_vector(bv);
}

void test_select_0_long_block(void) {
    BitVector *bv = construct_bit_vector("01");

    size_t index = select_0(bv, 0);
    size_t expected = 0;
    TEST_ASSERT_EQUAL(expected, index);

    destruct_bit_vector(bv);
}

void test_select_0_short_block(void) {
    BitVector *bv = construct_bit_vector(BIT_STR);

    // print_bit_vector(bv);
    size_t index = select_0(bv, 0);
    size_t expected = 0;
    TEST_ASSERT_EQUAL(expected, index);

    index = select_0(bv, 16);
    expected = 32;
    TEST_ASSERT_EQUAL(expected, index);

    index = select_0(bv, 31);
    expected = 62;
    TEST_ASSERT_EQUAL(expected, index);

    destruct_bit_vector(bv);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_construction_destruction);
    RUN_TEST(test_rank_1);
    RUN_TEST(test_rank_0);
    RUN_TEST(test_select_1_long_block);
    RUN_TEST(test_select_1_short_block);
    RUN_TEST(test_select_0_long_block);
    RUN_TEST(test_select_0_short_block);
    return UNITY_END();
}
