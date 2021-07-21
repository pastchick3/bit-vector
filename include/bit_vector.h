#ifndef BIT_VECTOR_H
#define BIT_VECTOR_H 1

#include <stddef.h>

typedef struct BitVector BitVector;

BitVector *construct_bit_vector(char const *const bits_str);
size_t rank_1(BitVector *bv, size_t index);
size_t rank_0(BitVector *bv, size_t index);
size_t select_1(BitVector *bv, size_t index);
size_t select_0(BitVector *bv, size_t index);
void destruct_bit_vector(BitVector *bv);
void print_bit_vector(BitVector *bv);

#endif
