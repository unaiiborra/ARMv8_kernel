#pragma once

#define FLAG_BIT(bit_pos) (1U << bit_pos)
#define SET_FLAG(flags, bit_pos, val)           \
    ((flags) = ((flags) & ~(1U << (bit_pos))) | \
               ((unsigned int)(!!(val)) << (bit_pos)))

#define GET_FLAG(flags, bit_pos) (bool)(((flags) >> (bit_pos)) & 1U)
