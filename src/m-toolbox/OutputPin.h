#ifndef MTBX_OUTPUT_PIN_H
#define MTBX_OUTPUT_PIN_H

#include "BitAccess.h"

template <
    uint8_t DataDirectionRegister,
    uint8_t PORTRegister,
    uint8_t PINRegister,
    uint8_t PinBit
>
class OutputPin {
private:
    BitAccess<DataDirectionRegister, PinBit> dataDirection;

    BitAccess<PORTRegister, PinBit> dataOutput;

    BitAccess<PINRegister, PinBit> dataReadOrToggle;

public:
    inline __attribute__((always_inline))
    OutputPin() {
        dataDirection.set();
    }

    inline __attribute__((always_inline))
    void set() {
        dataOutput.set();
    }

    inline __attribute__((always_inline))
    void clear() {
        dataOutput.clear();
    }

    inline __attribute__((always_inline))
    void set(bool isSet) {
        dataOutput.set(isSet);
    }
};

#endif // MTBX_OUTPUT_PIN_H
