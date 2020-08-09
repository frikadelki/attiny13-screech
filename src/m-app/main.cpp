#include <util/delay.h>

#include "../m-toolbox/Macro.h"
#include "../m-toolbox/InputPin.h"
#include "../m-toolbox/OutputPin.h"

// -------- NOTES DATA --------

// TODO: move to PROGMEM
const uint8_t NOTES_DIVISIONS[] = {
        255, //D1
        245, //A4
        230, //B5
        200, //C6
        181, //F6
        171, //G6
        161, //A7
        149, //B7
        143, //C7
        129, //D7
        113, //E7
        104, //F7
        86, //G7
        65, //A8
        41, //B8
        28, //C8
};

const uint8_t NOTES_COUNT = sizeof(NOTES_DIVISIONS) / sizeof(uint8_t);

// ----------------

// -------- INPUT HANDLER --------

static const uint8_t INPUT_FILTER_MAX = 0b1111u;
static const uint8_t INPUT_FILTER_NIBBLE_SIZE = 4u;

enum InputBtn {
    InputBtnL = 0,
    InputBtnR = 1,
};

class InputHandler {
private:
    InputPin<DDRB, PORTB, PINB, 4> inputL;

    InputPin<DDRB, PORTB, PINB, 3> inputR;

    uint8_t inputFilter = 0;

    bool inputRisingEdge[2] = { false, false };

public:
    InputHandler() = default;

    void readAndFilterInput() {
        inputRisingEdge[InputBtnL] = false;
        inputRisingEdge[InputBtnR] = false;

        uint8_t filterL = inputFilter >> INPUT_FILTER_NIBBLE_SIZE;
        if (inputL.read()) {
            if (filterL < INPUT_FILTER_MAX) {
                filterL++;
                if (filterL == INPUT_FILTER_MAX) {
                    inputRisingEdge[InputBtnL] = true;
                }
            }
        } else {
            if (filterL > 0) {
                filterL--;
            }
        }

        uint8_t filterR = inputFilter & INPUT_FILTER_MAX;
        if (inputR.read()) {
            if (filterR < INPUT_FILTER_MAX) {
                filterR++;
                if (filterR == INPUT_FILTER_MAX) {
                    inputRisingEdge[InputBtnR] = true;
                }
            }
        } else {
            if (filterR > 0) {
                filterR--;
            }
        }

        inputFilter = static_cast<uint8_t>(filterL << INPUT_FILTER_NIBBLE_SIZE) | filterR;
    }

    bool isRisingEdge(InputBtn button) {
        return inputRisingEdge[button];
    }

    bool isInputLHigh() {
        return INPUT_FILTER_MAX == (inputFilter >> INPUT_FILTER_NIBBLE_SIZE);
    }

    bool isInputLLow() {
        return 0 == (inputFilter >> INPUT_FILTER_NIBBLE_SIZE);
    }

    bool isInputRHigh() {
        return INPUT_FILTER_MAX == (inputFilter & INPUT_FILTER_MAX);
    }

    bool isInputRLow() {
        return 0 == (inputFilter & INPUT_FILTER_MAX);
    }
};

// ----------------

// -------- MAIN --------

class Main {
private:
    OutputPin<DDRB, PORTB, PINB, 0> blinkerPin;

    InputHandler inputHandler;

    uint8_t activeNote = 0;

    void cycle() {
        inputHandler.readAndFilterInput();

        if (inputHandler.isRisingEdge(InputBtnL) && activeNote > 0) {
            activeNote--;
            updateNote();
        }
        if (inputHandler.isRisingEdge(InputBtnR) && activeNote < NOTES_COUNT - 1) {
            activeNote++;
            updateNote();
        }

        _delay_ms(5);
    }

    void updateNote() {
        cli();

        uint8_t divisions = NOTES_DIVISIONS[activeNote];
        divisions = divisions > 0 ? divisions : 1;
        ACCESS_BYTE(OCR0A) = divisions;
        ACCESS_BYTE(OCR0B) = divisions / 2;

        sei();
    }

public:
    Main() {
        blinkerPin.clear();
        updateNote();
    }

    void run() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
        while (true) {
            cycle();
        }
#pragma clang diagnostic pop
    }

    void onWaveformCycleStep() {
        blinkerPin.set();
    }

    void onWaveformCycleEnd() {
        blinkerPin.clear();
    }
};

// ----------------

// -------- INIT --------

static Main *_mainCore;

ISR(TIM0_COMPB_vect) {
    _mainCore->onWaveformCycleStep();
}

ISR(TIM0_COMPA_vect) {
    _mainCore->onWaveformCycleEnd();
}

int main() {
    cli();

    static Main mainCore;
    _mainCore = &mainCore;

    // -------- configuring timer --------

    // set timer counter mode to CTC
    SET_BYTE_BIT(TCCR0A, WGM01);

    // set test frequency and waveform
    //ACCESS_BYTE(OCR0A) = 255u;
    //ACCESS_BYTE(OCR0B) = 191u;

    // enable compa & compb interrupts
    ACCESS_BYTE(TIMSK0) |= BIT_MASK(OCIE0A) | BIT_MASK(OCIE0B);

    // set pre-scaler to 1024 and start timer
    ACCESS_BYTE(TCCR0B) |= BIT_MASK(CS02) | BIT_MASK(CS00);

    // ----------------

    sei();

    mainCore.run();
}

// ----------------
