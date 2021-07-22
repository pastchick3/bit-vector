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

    size_t select_block_one_number;
    bool **select_block_types;
    size_t **select_blocks;
    void *****select_block_structures;


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
    bv->rank_1_blocks = malloc(bv->length / bv->block_length * sizeof(size_t) + 1);
    bv->rank_1_subblocks = malloc(bv->length / bv->subblock_length * sizeof(size_t) + 1);

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

static void build_long_select_block(BitVector *bv, size_t block_no, size_t start, size_t end, bool target)
{
    size_t *positions = malloc(bv->select_block_one_number * sizeof(size_t));
    size_t *pos_ptr = positions;
    for (size_t i = start; i < end; ++i)
    {
        if (is_bit_set(bv, i))
        {
            *pos_ptr = i;
            ++pos_ptr;
        }
    }
    bv->select_block_structures[target][block_no] = (void*)positions;
    return;
}

static void build_short_select_block(BitVector *bv, size_t block_no, size_t start, size_t end, bool target)
{
    size_t ***tree = malloc(8 * sizeof(size_t ***));

    size_t block_length = pow(bv->select_block_one_number, 2);
    size_t **bits = calloc(block_length, sizeof(size_t *));
    for (size_t i = start; i < end; ++i)
    {
        bits[i] = (size_t *)is_bit_set(bv, i);
    }
    tree[7] = bits;

    size_t subblock_length = sqrt(bv->select_block_one_number);
    size_t **level = malloc(block_length / subblock_length * sizeof(size_t *));
    size_t *table = malloc((bv->select_block_one_number + 1) * sizeof(size_t));
    size_t one_counter = 0;
    size_t sub_one_counter = 0;
    for (size_t i = 0; i < block_length; ++i)
    {
        
        if (i > 0 && !fmod(i, subblock_length))
        {
            table[0] = sub_one_counter;
            sub_one_counter = 0;
            level[i / subblock_length - 1] = table;
            table = malloc((bv->select_block_one_number + 1) * sizeof(size_t));
        }

        if (tree[7][i])
        {
            table[one_counter + 1] = i;
            ++one_counter;
            ++sub_one_counter;
        }
    }
    level[block_length / subblock_length - 1] = table;
    tree[6] = level;

    size_t ary_num = sqrt(sqrt(bv->select_block_one_number));
    size_t child_num = pow(sqrt(bv->select_block_one_number), 3);
    size_t node_num = child_num / ary_num;
    for (size_t i = 0; i < 6; ++i)
    {
        size_t **level = malloc(node_num * sizeof(size_t *));
        size_t *table = malloc((bv->select_block_one_number + 1) * sizeof(size_t));

        size_t one_counter = 0;
        size_t sub_one_counter = 0;
        for (size_t j = 0; j < child_num; ++j)
        {
            if (j > 0 && !fmod(j, ary_num))
            {
                table[0] = sub_one_counter;
                level[j / ary_num - 1] = table;
                table = malloc((bv->select_block_one_number + 1) * sizeof(size_t));
                sub_one_counter = 0;
            }

            size_t one_num = tree[5 - i + 1][j][0];
            
            for (size_t k = 0; k < one_num; ++k)
            {
                table[one_counter + k + 1] = fmod(j, ary_num);
                ++sub_one_counter;
            }
            one_counter += one_num;
        }
        table[0] = sub_one_counter;
        level[node_num - 1] = table;

        tree[5 - i] = level;
        child_num /= ary_num;
        node_num /= ary_num;
    }

    bv->select_block_structures[target][block_no] = (void*)tree;
    return;
}

static void build_select(BitVector *bv, bool target)
{
    // printf("***** %d\n", target);
    size_t sqrt_lgn = ceil(sqrt(log2(bv->length)));
    bv->select_block_one_number = pow(sqrt_lgn, 4);
    size_t block_length_boundary = pow(sqrt_lgn, 8);

    

    size_t max_block_num = ceil(bv->length / (double)bv->select_block_one_number);
    bv->select_block_types[target] = malloc(max_block_num * sizeof(bool));
    bv->select_blocks[target] = malloc(max_block_num * sizeof(size_t));
    bv->select_block_structures[target] = malloc(max_block_num * sizeof(void **));

    size_t one_num = 0;
    size_t block_no = 0;
    size_t start = 0;
    for (size_t i = 0; i < bv->length; ++i)
    {
        if (one_num == bv->select_block_one_number)
        {
            bool block_type = i - start > block_length_boundary;
            bv->select_block_types[target][block_no] = block_type;
            bv->select_blocks[target][block_no] = i;
            if (block_type)
            {
                build_long_select_block(bv, block_no, start, i, target);
            }
            else
            {
                build_short_select_block(bv, block_no, start, i, target);
            }
            one_num = 0;
            block_no += 1;
            start = i;
        }

        if (is_bit_set(bv, i))
        {
            one_num += 1;
        }
    }
    bool block_type = bv->length - start > block_length_boundary;
    bv->select_block_types[target][block_no] = block_type;
    bv->select_blocks[target][block_no] = bv->length;
    if (block_type)
    {
        build_long_select_block(bv, block_no, start, bv->length, target);
    }
    else
    {
        build_short_select_block(bv, block_no, start, bv->length, target);
    }
}

BitVector *construct_bit_vector(char const *const bits_str)
{
    BitVector *bv = malloc(sizeof(BitVector));

    bv->length = 0;
    char *cleaned_bits_str = malloc(strlen(bits_str) * sizeof(char));
    for (size_t index = 0; bits_str[index]; ++index)
    {
        if (bits_str[index] == '_' || bits_str[index] == ' ')
        {
        }
        else if (bits_str[index] == '0' || bits_str[index] == '1')
        {
            cleaned_bits_str[bv->length] = bits_str[index];
            bv->length += 1;
        }
        else
        {
            fprintf(stderr, "Unknown character `%c`in the input bit string.\n", bits_str[index]);
            exit(EXIT_FAILURE);
        }
    }

    size_t byte_num = ceil(bv->length / (float)8);
    uint8_t *bits = calloc(8, byte_num);
    bv->bits = bits;
    for (size_t i = 0; i < bv->length; ++i)
    {
        if (cleaned_bits_str[i] == '0')
        {
            set_bit(bv, i);
        }
    }

    free(cleaned_bits_str);

    bv->select_block_types = malloc(2*sizeof(bool*));
    bv->select_blocks = malloc(2*sizeof(size_t*));
    bv->select_block_structures = malloc(2*sizeof(void ****));
    
    build_select(bv, 0);

    for (size_t i = 0; i < byte_num; ++i) {
        bv->bits[i] = ~bv->bits[i];
    }

    build_rank_1(bv);
    build_select(bv, 1);

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

static size_t _select(BitVector *bv, size_t no, bool target)
{
    size_t block_no = no / bv->select_block_one_number;
    no = fmod(no, bv->select_block_one_number);
    
    if (bv->select_block_types[target][block_no])
    {
        
        size_t *positions = (size_t *)bv->select_block_structures[target][block_no];
        
        return positions[no];
    }
    else
    {
        size_t index = block_no?bv->select_blocks[target][block_no - 1]:0;
        no = fmod(no, bv->select_block_one_number);
        
        size_t ***tree = (size_t ***)bv->select_block_structures[target][block_no];

        size_t child_no = 0;
        size_t sibling_num = 0;
        size_t ary_num = sqrt(sqrt(bv->select_block_one_number));
        for (size_t i = 0; i < 7; ++i)
        {
            
            size_t node_no = child_no + sibling_num * ary_num;
            child_no = tree[i][node_no][no + 1];
            sibling_num = node_no;
        }

        // printf("*****  %ld, %ld\n",  child_no, sibling_num);
        size_t bit_no = child_no;// + sibling_num * sqrt(bv->select_block_one_number);

        

        index += bit_no;
        return index;
    }
}

size_t select_1(BitVector *bv, size_t no) {
    return _select(bv, no, 1);
}

size_t select_0(BitVector *bv, size_t no)
{
    
    return _select(bv, no, 0);
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

void print_bit_vector(BitVector *bv)
{
    // printf("***** Bit Vector at %p *****\n\n", bv);

    // printf("%zu Bits: ", bv->length);
    // size_t byte_num = ceil(bv->length / (float)8);
    // for (size_t i = 0; i < byte_num; ++i)
    // {
    //     printf("0x%hhx ", bv->bits[i]);
    // }
    // printf("\n\n");

    // printf("----- Rank -----\n\n");

    // printf("Block Length: %zu\tSubblock Length: %zu\n\n", bv->block_length, bv->subblock_length);

    // printf("Blocks: ");
    // size_t block_num = bv->length / bv->block_length + 1;
    // for (size_t i = 0; i < block_num; ++i)
    // {
    //     printf("%zu ", bv->rank_1_blocks[i]);
    // }
    // printf("\n\n");

    // printf("Subblocks: ");
    // size_t subblock_num = bv->length / bv->subblock_length + 1;
    // for (size_t i = 0; i < subblock_num; ++i)
    // {
    //     printf("%zu ", bv->rank_1_subblocks[i]);
    // }
    // printf("\n\n");

    // printf("Subblock Pattern Index Map:\n");
    // for (size_t i = 0; i < pow(2, bv->subblock_length); ++i)
    // {
    //     printf("Pattern 0x%zx:", i);
    //     for (size_t j = 0; j < bv->subblock_length; ++j)
    //     {
    //         printf(" %zu", bv->rank_1_subblock_table[i][j]);
    //     }
    //     printf("\n\n");
    // }

    // printf("----- Select -----\n\n");

    printf("Number of 1's in a Block: %zu\n\n", bv->select_block_one_number);

    // size_t select_block_num = ceil(bv->length / (double)bv->select_block_one_number);

    // printf("Block Type:");
    // for (size_t i = 0; i < select_block_num; ++i)
    // {
    //     printf(" %d", bv->select_block_types[target][i]);
    // }
    // printf("\n\n");

    // printf("Block Structure:\n");
    // for (size_t i = 0; i < 1; ++i) {
    //     for (size_t j = 0; j < 17; ++j) {
    //         printf(" %ld", ((size_t****)bv->select_block_structures[target])[0][5][i][j]);
    //     }
    //     printf("\n");
    // }

    // printf("\n");

    // for (size_t i = 0; i < 3; ++i) {
    //     for (size_t j = 0; j < 17; ++j) {
    //         printf(" %ld", ((size_t****)bv->select_block_structures[target])[0][6][i][j]);
    //     }
    //     printf("\n");
    // }
    // printf("\n\n");

    // printf("***** %ld\n", sub_one_counter);

    // printf("**********\n");
}
