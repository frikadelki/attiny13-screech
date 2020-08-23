#ifndef MTBX_BIT_ACCESS_H
#define MTBX_BIT_ACCESS_H

#include "Macro.h"

template <uint8_t Register, uint8_t Bit>
class BitAccess {
public:
    inline __attribute__((always_inline))
    static void set() {
        SET_BYTE_BIT(Register, Bit);
    }

    inline __attribute__((always_inline))
    static void clear() {
        CLEAR_BYTE_BIT(Register, Bit);
    }

    inline __attribute__((always_inline))
    static void set(bool isSet) {
        isSet ? set() : clear();
    }

    inline __attribute__((always_inline))
    static void set(uint8_t setBit) {
        (0 != setBit) ? set() : clear();
    }

    inline __attribute__((always_inline))
    static bool isSet() {
        return IS_BYTE_BIT_SET(Register, Bit);
    }
};

#endif // MTBX_BIT_ACCESS_H
