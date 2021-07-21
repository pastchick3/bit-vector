#include "../include/bit_vector.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

struct BitVector
{
    size_t length;
    uint8_t *bits;
    size_t block_length;
    size_t subblock_length;
    size_t *rank_1_blocks;
    size_t *rank_1_subblocks;
    size_t **rank_1_subblock_table;
};

static void find_bit(BitVector *bv, size_t index, uint8_t **byte, uint8_t *mask)
{
    size_t byte_num = index / 8;
    size_t mod = fmod(index, 8);
    *byte = bv->bits + byte_num;
    *mask = 0b10000000 >> mod;
}

static bool is_bit_set(BitVector *bv, size_t index)
{
    uint8_t *byte = NULL;
    uint8_t mask = 0;
    find_bit(bv, index, &byte, &mask);
    return *byte & mask;
}

static void set_bit(BitVector *bv, size_t index)
{
    uint8_t *byte = NULL;
    uint8_t mask = 0;
    find_bit(bv, index, &byte, &mask);
    *byte |= mask;
}

static void build_rank_1(BitVector *bv)
{
    size_t lg = ceil(log2(bv->length));
    lg = fmod(lg, 2) ? lg + 1 : lg;
    bv->block_length = pow(lg, 2);
    bv->subblock_length = lg / 2;
    bv->rank_1_blocks = malloc(bv->length / bv->block_length * sizeof(size_t)+1);
    bv->rank_1_subblocks = malloc(bv->length / bv->subblock_length * sizeof(size_t)+1);

    size_t block_counter = 0;
    size_t subblock_counter = 0;
    for (size_t i = 0; i < bv->length; ++i)
    {
        if (!fmod(i, bv->block_length))
        {
            size_t block_no = i / bv->block_length;
            bv->rank_1_blocks[block_no] = block_counter;
            subblock_counter = 0;
        }

        if (!fmod(i, bv->subblock_length))
        {
            size_t subblock_no = i / bv->subblock_length;
            bv->rank_1_subblocks[subblock_no] = subblock_counter;
        }

        if (is_bit_set(bv, i))
        {
            block_counter += 1;
            subblock_counter += 1;
        }
    }

    size_t pattern_no = pow(2, bv->subblock_length);
    bv->rank_1_subblock_table = malloc(pattern_no * sizeof(size_t *));
    for (size_t pattern = 0; pattern < pattern_no; ++pattern)
    {
        size_t *pattern_index_map = malloc(bv->subblock_length * sizeof(size_t));
        size_t counter = 0;
        for (size_t i = 0; i < bv->subblock_length; ++i)
        {
            counter += 1 & (pattern >> (bv->subblock_length - i));
            pattern_index_map[i] = counter;
        }
        bv->rank_1_subblock_table[pattern] = pattern_index_map;
    }

    return;
}

BitVector *construct_bit_vector(char const *const bits_str)
{
    BitVector *bv = malloc(sizeof(BitVector));

    bv->length = 0;
    char *cleaned_bits_str = malloc(strlen(bits_str)*sizeof(char));
    for (size_t index = 0;bits_str[index];++index) {
        if (bits_str[index] == '_' || bits_str[index] == ' ') {

        } else if (bits_str[index] == '0' || bits_str[index] == '1') {
            cleaned_bits_str[bv->length] = bits_str[index];
            bv->length += 1;
        } else {
            fprintf(stderr, "Unknown character `%c`in the input bit string.\n", bits_str[index]);
            exit( EXIT_FAILURE );
        }
    }
    
    size_t byte_num = ceil(bv->length / (float)8);
    uint8_t *bits = calloc(8, byte_num);
    bv->bits = bits;
    for (size_t i = 0; i < bv->length; ++i)
    {
        if (cleaned_bits_str[i] == '1')
        {
            set_bit(bv, i);
        }
    }

    free(cleaned_bits_str);

    build_rank_1(bv);

    return bv;
}

size_t rank_1(BitVector *bv, size_t index)
{
    size_t rank = 0;

    size_t block_no = index / bv->block_length;
    rank += bv->rank_1_blocks[block_no];


    size_t subblock_no = index / bv->subblock_length;
    rank += bv->rank_1_subblocks[subblock_no];

    

    size_t pattern = 0;
    size_t subblock_start = subblock_no * bv->subblock_length;
    for (size_t i = 0; i < bv->subblock_length; ++i)
    {
        if (subblock_start + i < bv->length && is_bit_set(bv, subblock_start + i))
        {
            pattern += 1;
        }
        pattern <<= 1;
    }
    pattern >>= 1;
    size_t mod = fmod(index, bv->subblock_length);
    rank += bv->rank_1_subblock_table[pattern][mod];

    return rank;
}

size_t rank_0(BitVector *bv, size_t index)
{
    return index - rank_1(bv, index);
}

size_t select_1(BitVector *bv, size_t index)
{
    return index;
}

size_t select_0(BitVector *bv, size_t index)
{
    return index;
}

void destruct_bit_vector(BitVector *bv)
{
    free(bv->bits);
    free(bv->rank_1_blocks);
    free(bv->rank_1_subblocks);
    for (size_t i = 0; i < pow(2, bv->subblock_length); ++i)
    {
        free(bv->rank_1_subblock_table[i]);
    }
    free(bv->rank_1_subblock_table);
    return;
}

void print_bit_vector(BitVector *bv) {
    printf("***** Bit Vector at %p *****\n", bv);
    printf("Total Length: %zu\tBlock Length: %zu\tSubblock Length: %zu\n", bv->length, bv->block_length, bv->subblock_length);
    printf("\n");

    printf("Bits: ");
    size_t byte_num = ceil(bv->length / (float)8);
    for (size_t i = 0; i < byte_num; ++i) {
        printf("0x%hhx ", bv->bits[i]);
    }
    printf("\n");
    printf("\n");

    printf("Blocks: ");
    size_t block_num = bv->length / bv->block_length + 1;
    for (size_t i = 0; i < block_num; ++i) {
        printf("%zu ", bv->rank_1_blocks[i]);
    }
    printf("\n");
    printf("\n");

    printf("Subblocks: ");
    size_t subblock_num = bv->length / bv->subblock_length + 1;
    for (size_t i = 0; i < subblock_num; ++i) {
        printf("%zu ", bv->rank_1_subblocks[i]);
    }
    printf("\n");
    printf("\n");


    printf("Subbloakc Pattern Index Map:\n");
    for (size_t i = 0; i < pow(2, bv->subblock_length); ++i) {
        printf("Pattern 0x%zx:", i);
        for (size_t j = 0; j < bv->subblock_length; ++j) {
            printf(" %zu", bv->rank_1_subblock_table[i][j]);
        }
        printf("\n");
    }
    printf("**********\n");
}
