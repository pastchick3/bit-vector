#include "unity.h"
#include "bit_vector.h"

char const * const BIT_STR = "01010101_01010101_01010101_01010101_01010101_01010101_01010101_01010101";

void setUp(void) {}
void tearDown(void) {}

void test_construction_destruction(void) {
    BitVector *bv = construct_bit_vector(BIT_STR);
    print_bit_vector(bv);
    destruct_bit_vector(bv);
}

void test_rank_1(void) {
    BitVector *bv = construct_bit_vector(BIT_STR);

    size_t rank = rank_1(bv, 0);
    size_t expected = 0;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_1(bv, 32);
    expected = 16;
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

    rank = rank_0(bv, 32);
    expected = 16;
    TEST_ASSERT_EQUAL(expected, rank);

    rank = rank_0(bv, 64);
    expected = 32;
    TEST_ASSERT_EQUAL(expected, rank);

    destruct_bit_vector(bv);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_construction_destruction);
    RUN_TEST(test_rank_1);
    RUN_TEST(test_rank_0);
    return UNITY_END();
}
