#ifndef BIT_VECTOR_H
#define BIT_VECTOR_H 1

#include <stddef.h>

typedef struct BitVector BitVector;

BitVector *construct_bit_vector(char const *const bits_str);
void destruct_bit_vector(BitVector *bv);

size_t rank_one(BitVector *bv, size_t index);
size_t rank_zero(BitVector *bv, size_t index);
size_t select_one(BitVector *bv, size_t index);
size_t select_zero(BitVector *bv, size_t index);

#endif
