#pragma once

#include <stdint.h>

// Returns the signed 23-bit modular difference a-b in range [-2^22, 2^22-1].
// Contract: intended for use when a and b are within 2^22 of each other.
int32_t a_minus_b_mod23(uint32_t a, uint32_t b);
