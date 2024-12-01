#include "bitarray.h"

#if BIT_LENGTH <= 9
static inline uint16_t bitholder_get(const uint8_t *bits) {
    return (bits[0] << 8) | bits[1];
}

static inline void bitholder_set(uint8_t *bits, uint16_t value) {
    bits[0] = value >> 8;
    bits[1] = value;
}

#else
static uint32_t bitholder_get(const uint8_t *bits) {
    uint32_t result = 0;

    for (uint8_t i = 0; i < 4; ++i) {
        result <<= 8;
        result |= bits[i];
    }
    return result;
}

static void bitholder_set(uint8_t *bits, uint32_t value) {
    for (int8_t i = 3; i >= 0; --i) {
        bits[i] = value;
        value >>= 8;
    }
}
#endif

bitvalue_t bitarray_get(const uint8_t *bits, uint16_t index) {
    index *= BIT_LENGTH;
#if BIT_LENGTH <= 9
    return (uint16_t)(bitholder_get(&bits[index >> 3]) << (index & 0x07)) >> (16 - BIT_LENGTH);
#else
    return (bitholder_get(&bits[index >> 3]) << (index & 0x07)) >> (32 - BIT_LENGTH);
#endif
}

void bitarray_set(uint8_t *bits, uint16_t index, bitvalue_t value) {
#if BIT_LENGTH <= 9
    uint16_t data;
#else
    uint32_t data;
#endif

    index *= BIT_LENGTH;
    data = bitholder_get(&bits[index >> 3]);
#if BIT_LENGTH <= 9
    data &= ~((uint16_t)((uint16_t)-1 << (16 - BIT_LENGTH)) >> (index & 0x07));
    data |= (value << (16 - BIT_LENGTH - (index & 0x07)));
#else
    data &= ~(((uint32_t)-1 << (32 - BIT_LENGTH)) >> (index & 0x07));
    data |= (value << (32 - BIT_LENGTH - (index & 0x07)));
#endif
    bitholder_set(&bits[index >> 3], data);
}
