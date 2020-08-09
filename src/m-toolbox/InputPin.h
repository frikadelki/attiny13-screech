#ifndef MTBX_INPUT_PIN_H
#define MTBX_INPUT_PIN_H

#include "BitAccess.h"

template <
    uint8_t DataDirectionRegister,
    uint8_t PORTRegister,
    uint8_t PINRegister,
    uint8_t PinBit
>
class InputPin {
private:
    BitAccess<DataDirectionRegister, PinBit> dataDirection;

    BitAccess<PINRegister, PinBit> dataInput;

    BitAccess<PORTRegister, PinBit> pinPullup;

public:
    inline __attribute__((always_inline))
    InputPin() {
        dataDirection.clear();
    }

    inline __attribute__((always_inline))
    bool read() {
        return dataInput.isSet();
    }
};

#endif //MTBX_INPUT_PIN_H
