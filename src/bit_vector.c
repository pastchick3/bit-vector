#include "../include/bit_vector.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

/********** Declarations of Private Functions **********/

static size_t select_target(BitVector *bv, size_t index, bool target);
static void build_rank(BitVector *bv);
static void build_select(BitVector *bv, bool target);
static size_t *build_long_select_structure(BitVector *bv, size_t start, size_t end);
static size_t ***build_short_select_structure(BitVector *bv, size_t start, size_t end);
static void find_bit(BitVector *bv, size_t index, uint8_t **byte, uint8_t *mask);
static bool is_bit_set(BitVector *bv, size_t index);
static void set_bit(BitVector *bv, size_t index);

/********** Definitions of `BitVector` and Public Functions **********/

struct BitVector
{
    // The original bit string.
    size_t length;
    uint8_t *bits;

    // Rank structures.
    size_t rank_block_length;
    size_t rank_subblock_length;
    size_t *rank_blocks;
    size_t *rank_subblocks;
    size_t **rank_subblock_table;

    // Select structures.
    size_t select_block_one_number;
    size_t *select_block_number;
    bool **select_block_types;
    size_t **select_blocks;
    void ***select_block_structures;
};

BitVector *construct_bit_vector(char const *const bits_str)
{
    BitVector *bv = malloc(sizeof(BitVector));

    // Remove "_" and " " separators in the original bit string.
    bv->length = 0;
    char *cleaned_bits_str = malloc(strlen(bits_str) * sizeof(char));
    for (size_t i = 0; bits_str[i]; ++i)
    {
        if (bits_str[i] == '_' || bits_str[i] == ' ')
        {
        }
        else if (bits_str[i] == '0' || bits_str[i] == '1')
        {
            cleaned_bits_str[bv->length++] = bits_str[i];
        }
        else
        {
            fprintf(stderr, "Error: Unknown character `%c` in the input bit string.\n", bits_str[i]);
            exit(EXIT_FAILURE);
        }
    }

    // Build a flipped bit string first.
    size_t byte_num = ceil(bv->length / (double)8);
    bv->bits = calloc(byte_num, 8);
    for (size_t i = 0; i < bv->length; ++i)
    {
        if (cleaned_bits_str[i] == '0')
        {
            set_bit(bv, i);
        }
    }

    // Use the flipped bit string to build select_0 structures.
    bv->select_block_number = malloc(2 * sizeof(size_t));
    bv->select_block_types = malloc(2 * sizeof(bool *));
    bv->select_blocks = malloc(2 * sizeof(size_t *));
    bv->select_block_structures = malloc(2 * sizeof(void ****));
    build_select(bv, 0);

    // Restore a normal bit string.
    for (size_t i = 0; i < byte_num; ++i)
    {
        bv->bits[i] = ~bv->bits[i];
    }

    // Build rank_1 and select_1 structures.
    build_rank(bv);
    build_select(bv, 1);

    free(cleaned_bits_str);
    return bv;
}

void destruct_bit_vector(BitVector *bv)
{
    // Free the bit string.
    free(bv->bits);

    // Free the rank structures.
    free(bv->rank_blocks);
    free(bv->rank_subblocks);
    size_t pattern_num = pow(2, bv->rank_subblock_length);
    for (size_t i = 0; i < pattern_num; ++i)
    {
        free(bv->rank_subblock_table[i]);
    }
    free(bv->rank_subblock_table);

    // Free the select structures.
    for (size_t target = 0; target < 2; ++target)
    {
        for (size_t block = 0; block < bv->select_block_number[target]; ++block)
        {
            if (bv->select_block_types[target][block])
            {
                size_t *positions = bv->select_block_structures[target][block];
                free(positions);
            }
            else
            {
                // Free 1st to 7th levels.
                size_t ***tree = bv->select_block_structures[target][block];
                size_t node_num = 1;
                size_t ary_num = sqrt(sqrt(bv->select_block_one_number));
                for (size_t level = 0; level < 7; ++level)
                {
                    for (size_t node = 0; node < node_num; ++node)
                    {
                        free(tree[level][node]);
                    }
                    free(tree[level]);
                    node_num *= ary_num;
                }

                // Remember we do not really allocate memory for bits in the 8th level,
                // so we can just free the whole level.
                free(tree[7]);

                free(tree);
            }
        }

        free(bv->select_block_types[target]);
        free(bv->select_blocks[target]);
        free(bv->select_block_structures[target]);
    }

    free(bv->select_block_types);
    free(bv->select_blocks);
    free(bv->select_block_structures);
}

size_t rank_one(BitVector *bv, size_t index)
{
    size_t rank = 0;

    // Add ranks in previous blocks.
    size_t block = index / bv->rank_block_length;
    rank += bv->rank_blocks[block];

    // Add ranks in previous subblocks.
    size_t subblock = index / bv->rank_subblock_length;
    rank += bv->rank_subblocks[subblock];

    // Extract the remaining bit pattern.
    size_t pattern = 0;
    size_t start = subblock * bv->rank_subblock_length;
    for (size_t i = 0; i < bv->rank_subblock_length; ++i)
    {
        if (start + i < bv->length && is_bit_set(bv, start + i))
        {
            pattern += 1;
        }
        pattern <<= 1;
    }
    pattern >>= 1;

    // Add ranks corresponding to the bit pattern.
    index = fmod(index, bv->rank_subblock_length);
    rank += bv->rank_subblock_table[pattern][index];

    return rank;
}

size_t rank_zero(BitVector *bv, size_t index)
{
    return index - rank_one(bv, index);
}

size_t select_one(BitVector *bv, size_t index)
{
    return select_target(bv, index, 1);
}

size_t select_zero(BitVector *bv, size_t index)
{
    return select_target(bv, index, 0);
}

/********** Definitions for Private Functions **********/

static size_t select_target(BitVector *bv, size_t index, bool target)
{
    size_t block = index / bv->select_block_one_number;
    index = fmod(index, bv->select_block_one_number);

    if (bv->select_block_types[target][block])
    {
        // Find a long block.
        size_t *positions = bv->select_block_structures[target][block];
        return positions[index];
    }
    else
    {
        // Find a short block.

        // Add indexes from previous blocks.
        size_t target_index = block ? bv->select_blocks[target][block - 1] : 0;
        index = fmod(index, bv->select_block_one_number);

        // Go down to the 7th level of the tree to find the target bit index.
        size_t ***tree = bv->select_block_structures[target][block];
        size_t child = 0;
        size_t sibling_num = 0;
        size_t ary_num = sqrt(sqrt(bv->select_block_one_number));
        for (size_t i = 0; i < 7; ++i)
        {
            size_t node = child + sibling_num * ary_num;
            child = tree[i][node][index + 1];
            sibling_num = node;
        }

        return target_index + child;
    }
}

static void build_rank(BitVector *bv)
{
    size_t lgn = ceil(log2(bv->length));
    lgn = fmod(lgn, 2) ? lgn + 1 : lgn;
    bv->rank_block_length = pow(lgn, 2);
    bv->rank_subblock_length = lgn / 2;
    bv->rank_blocks = malloc(bv->length / bv->rank_block_length * sizeof(size_t) + 1);
    bv->rank_subblocks = malloc(bv->length / bv->rank_subblock_length * sizeof(size_t) + 1);

    size_t counter = 0;
    size_t sub_counter = 0;
    for (size_t i = 0; i < bv->length; ++i)
    {
        if (!fmod(i, bv->rank_block_length))
        {
            // Find a block.
            size_t block = i / bv->rank_block_length;
            bv->rank_blocks[block] = counter;
            counter = 0;
        }

        if (!fmod(i, bv->rank_subblock_length))
        {
            // Find a subblock.
            size_t subblock = i / bv->rank_subblock_length;
            bv->rank_subblocks[subblock] = counter;
        }

        if (is_bit_set(bv, i))
        {
            counter += 1;
            sub_counter += 1;
        }
    }

    // Build the final block.
    if (!fmod(bv->length, bv->rank_block_length))
    {
        size_t block = bv->length / bv->rank_block_length;
        bv->rank_blocks[block] = counter;
    }

    // Build the final subblock.
    if (!fmod(bv->length, bv->rank_subblock_length))
    {
        size_t subblock = bv->length / bv->rank_subblock_length;
        bv->rank_subblocks[subblock] = sub_counter;
    }

    // Build the table that maps ranks to pattern/index.
    size_t pattern_num = pow(2, bv->rank_subblock_length);
    bv->rank_subblock_table = malloc(pattern_num * sizeof(size_t *));
    for (size_t pattern = 0; pattern < pattern_num; ++pattern)
    {
        size_t *pattern_index_map = malloc(bv->rank_subblock_length * sizeof(size_t));
        size_t counter = 0;
        for (size_t i = 0; i < bv->rank_subblock_length; ++i)
        {
            counter += 1 & (pattern >> (bv->rank_subblock_length - i));
            pattern_index_map[i] = counter;
        }
        bv->rank_subblock_table[pattern] = pattern_index_map;
    }
}

static void build_select(BitVector *bv, bool target)
{
    size_t sqrt_lgn = ceil(sqrt(log2(bv->length)));
    bv->select_block_one_number = pow(sqrt_lgn, 4);
    size_t block_length_boundary = pow(sqrt_lgn, 8);

    size_t max_block_num = ceil(bv->length / (double)bv->select_block_one_number);
    bv->select_block_types[target] = malloc(max_block_num * sizeof(bool));
    bv->select_blocks[target] = malloc(max_block_num * sizeof(size_t));
    bv->select_block_structures[target] = malloc(max_block_num * sizeof(void **));

    size_t counter = 0;
    size_t block = 0;
    size_t start = 0;
    for (size_t i = 0; i < bv->length; ++i)
    {
        if (counter == bv->select_block_one_number)
        {
            // Find a block.
            bool block_type = i - start > block_length_boundary;
            bv->select_block_types[target][block] = block_type;
            bv->select_blocks[target][block] = i;
            if (block_type)
            {
                // Find a long block.
                size_t *positions = build_long_select_structure(bv, start, i);
                bv->select_block_structures[target][block] = positions;
            }
            else
            {
                // Find a short block.
                size_t ***tree = build_short_select_structure(bv, start, i);
                bv->select_block_structures[target][block] = tree;
            }
            counter = 0;
            block += 1;
            start = i;
        }

        if (is_bit_set(bv, i))
        {
            counter += 1;
        }
    }

    // Add the final block.
    bool block_type = bv->length - start > block_length_boundary;
    bv->select_block_types[target][block] = block_type;
    bv->select_blocks[target][block] = bv->length;
    if (block_type)
    {
        size_t *positions = build_long_select_structure(bv, start, bv->length);
        bv->select_block_structures[target][block] = positions;
    }
    else
    {
        size_t ***tree = build_short_select_structure(bv, start, bv->length);
        bv->select_block_structures[target][block] = tree;
    }

    // Record the number of blocks for proper destruction.
    bv->select_block_number[target] = block + 1;
}

static size_t *build_long_select_structure(BitVector *bv, size_t start, size_t end)
{
    size_t *positions = malloc(bv->select_block_one_number * sizeof(size_t));
    size_t *pos_ptr = positions;
    for (size_t i = start; i < end; ++i)
    {
        if (is_bit_set(bv, i))
        {
            *pos_ptr++ = i;
        }
    }
    return positions;
}

static size_t ***build_short_select_structure(BitVector *bv, size_t start, size_t end)
{
    size_t block_length = pow(bv->select_block_one_number, 2);
    size_t subblock_length = sqrt(bv->select_block_one_number);

    // The tree structure has 8 levels.
    size_t ***tree = malloc(8 * sizeof(size_t **));

    // The 8th level is the original bits, padding with 0 to the length of (log2(n))^4.
    // The `size_t *` type of each bit is only for type consistency.
    size_t **bits = malloc(block_length * sizeof(size_t *));
    for (size_t i = start; i < end; ++i)
    {
        bits[i] = (size_t *)is_bit_set(bv, i);
    }
    tree[7] = bits;

    // Other levels consist of a sqrt(log2(n))-ary tree.
    // Each node is an array, whose first element is the number of ones and the remaining elements
    // are child indexes for the query indexes that equal to the elements' indexes minus 1.

    // We need to build the 7th level separately because it has log2(n) bit children.
    size_t **level = malloc(block_length / subblock_length * sizeof(size_t *));
    size_t *table = malloc((bv->select_block_one_number + 1) * sizeof(size_t));
    size_t counter = 0;
    size_t sub_counter = 0;
    for (size_t i = 0; i < block_length; ++i)
    {
        if (i > 0 && !fmod(i, subblock_length))
        {
            // Find a subblock.
            table[0] = sub_counter;
            sub_counter = 0;
            level[i / subblock_length - 1] = table;
            table = malloc((bv->select_block_one_number + 1) * sizeof(size_t));
        }

        if (tree[7][i])
        {
            table[counter + 1] = i;
            ++counter;
            ++sub_counter;
        }
    }

    //Add the final subblock.
    table[0] = sub_counter;
    level[block_length / subblock_length - 1] = table;
    tree[6] = level;

    // Build remaining levels.
    size_t ary_num = sqrt(sqrt(bv->select_block_one_number));
    size_t child_num = pow(sqrt(bv->select_block_one_number), 3);
    size_t node_num = child_num / ary_num;
    for (size_t i = 0; i < 6; ++i)
    {
        size_t **level = malloc(node_num * sizeof(size_t *));
        size_t *table = malloc((bv->select_block_one_number + 1) * sizeof(size_t));
        size_t counter = 0;
        size_t sub_counter = 0;
        for (size_t child = 0; child < child_num; ++child)
        {
            if (child > 0 && !fmod(child, ary_num))
            {
                // Finish processing all children of one node.
                table[0] = sub_counter;
                sub_counter = 0;
                level[child / ary_num - 1] = table;
                table = malloc((bv->select_block_one_number + 1) * sizeof(size_t));
            }

            // Process one child.
            size_t one_num = tree[(5 - i) + 1][child][0];
            for (size_t j = 0; j < one_num; ++j)
            {
                table[counter + j + 1] = fmod(child, ary_num);
                ++sub_counter;
            }
            counter += one_num;
        }

        // Add the final node.
        table[0] = sub_counter;
        level[node_num - 1] = table;

        tree[5 - i] = level;
        child_num /= ary_num;
        node_num /= ary_num;
    }

    return tree;
}

static void find_bit(BitVector *bv, size_t index, uint8_t **byte, uint8_t *mask)
{
    size_t byte_num = index / 8;
    index = fmod(index, 8);
    *byte = bv->bits + byte_num;
    *mask = 128 >> index;
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
