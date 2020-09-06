#ifndef MTBX_COMBO_PIN_H
#define MTBX_COMBO_PIN_H

#include "BitAccess.h"
#include "Utils.h"

// this assumes the following setup
/*
Vcc-------+
          |
         [ ] 330 Ohm,1K
          |
         ---
          V  LED (IO low : Led active)
         -+-
IO--------+------------+
                       |
                       \  Switch
                       |
Ground-----------------+
*/
// output mode is used only to provide output for turned on LED (0 on IO pin)
// input mode is used for input as well as for turned off LED
// this avoids high state on IO pin connected directly to ground via pushed button
template <
        uint8_t DDRegister,
        uint8_t PORTRegister,
        uint8_t PINRegister,
        uint8_t PinBit
>
class ComboPin {
private:
    typedef BitAccess<DDRegister, PinBit> dataDirection;
    //BitAccess<DDRegister, PinBit> dataDirection;

    // when set as input this port will control pull-up resistor
    typedef BitAccess<PORTRegister, PinBit> dataOutput;
    //BitAccess<PORTRegister, PinBit> dataOutput;

    typedef BitAccess<PINRegister, PinBit> dataInput;
    //BitAccess<PINRegister, PinBit> dataInput;

    ComboPin() = default;

public:
    inline __attribute__((always_inline))
    static void init() {
        setAsInput();
        dataOutput::clear();
    }

    inline __attribute__((always_inline))
    static void inputPrime() {
        setAsInput();
    }

    // to guarantee reading correct input level, delay must be inserted
    // between priming pin for input and reading the value
    inline __attribute__((always_inline))
    static bool inputRead() {
        return !dataInput::isSet();
    }

    inline __attribute__((always_inline))
    static void outputSet() {
        setAsOutput();
    }

    inline __attribute__((always_inline))
    static void outputClear() {
        setAsInput();
    }

    inline __attribute__((always_inline))
    static void outputSet(const bool isSet) {
        if (isSet) {
            outputSet();
        } else {
            outputClear();
        }
    }

private:
    inline __attribute__((always_inline))
    static void setAsInput() {
        dataDirection::clear();
    }

    inline __attribute__((always_inline))
    static void setAsOutput() {
        dataDirection::set();
    }
};

__attribute__((always_inline))
void ComboPinWaitInputSettle() {
    fixedDelayLong();
}

#endif // MTBX_COMBO_PIN_H
