#pragma once

#include <stdint.h>

#define BIT_LENGTH  10

#if BIT_LENGTH <= 8
typedef uint8_t bitvalue_t;
#elif BIT_LENGTH <= 16
typedef uint16_t bitvalue_t;
#elif BIT_LENGTH <= 25
typedef uint32_t bitvalue_t;
#else
#error "BitArray does not support more than 25 bits!"
#endif

#define BITARRAY_SIZE(s)   (((s) * BIT_LENGTH + 7) >> 3)

bitvalue_t bitarray_get(const uint8_t *bits, uint16_t index);
void bitarray_set(uint8_t *bits, uint16_t index, bitvalue_t value);
