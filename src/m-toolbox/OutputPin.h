#ifndef MTBX_OUTPUT_PIN_H
#define MTBX_OUTPUT_PIN_H

#include "BitAccess.h"

template <
    uint8_t DDRegister,
    uint8_t PORTRegister,
    uint8_t PINRegister,
    uint8_t PinBit
>
class OutputPin {
private:
    typedef BitAccess<DDRegister, PinBit> dataDirection;

    typedef BitAccess<PORTRegister, PinBit> dataOutput;

    typedef BitAccess<PINRegister, PinBit> dataReadOrToggle;

public:
    inline __attribute__((always_inline))
    static void init() {
        dataDirection::set();
    }

    inline __attribute__((always_inline))
    static void set() {
        dataOutput::set();
    }

    inline __attribute__((always_inline))
    static void clear() {
        dataOutput::clear();
    }

    inline __attribute__((always_inline))
    static void set(bool isSet) {
        dataOutput::set(isSet);
    }
};

#endif // MTBX_OUTPUT_PIN_H
