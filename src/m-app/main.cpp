#include <util/delay.h>

#include "../m-toolbox/Macro.h"
#include "../m-toolbox/InputPin.h"
#include "../m-toolbox/OutputPin.h"

#include <avr/pgmspace.h>

// -------- NOTES DATA --------

const uint8_t NOTES_DIVISIONS[] PROGMEM = {
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

// -------- WAVEFORM DATA --------

const uint8_t WAVEFORMS[] PROGMEM = {
        0b00000000,

        0b11000000,
        0b11110000,
        0b11111100,
        0b11111111,

        0b10001000,
        0b11001100,
        0b11101110,

        0b11110010,
        0b11110001,

        0b10010010,
        0b10101010,
};

const uint8_t WAVEFORMS_COUNT = sizeof(WAVEFORMS) / sizeof(uint8_t);

const uint8_t WAVEFORM_LENGTH = 8u;

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

    uint8_t activeWaveform = 0;

    uint8_t waveformCycleStep = 0;

    void cycle() {
        inputHandler.readAndFilterInput();

        if (inputHandler.isRisingEdge(InputBtnL)) {
            activeNote = (activeNote + 1) % NOTES_COUNT;
            updateNote();
        }
        if (inputHandler.isRisingEdge(InputBtnR)) {
            activeWaveform = (activeWaveform + 1) % WAVEFORMS_COUNT;
        }

        _delay_ms(10);
    }

    void updateNote() {
        cli();

        uint8_t divisions = pgm_read_byte(&(NOTES_DIVISIONS[activeNote]));
        divisions = divisions > 0 ? divisions : 1;
        ACCESS_BYTE(OCR0A) = divisions;

        onWaveformCycleEnd();

        // clear any pending timer interrupts and restart timer fom zero
        ACCESS_BYTE(TIFR0) = BIT_MASK(OCF0B) | BIT_MASK(OCF0A) | BIT_MASK(TOV0);
        ACCESS_BYTE(TCNT0) = 0;

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

    uint8_t waveformStepDivisions() {
        return ACCESS_BYTE(OCR0A) / (WAVEFORM_LENGTH + 1);
    }

    void onWaveformCycleStep() {
        // this actually should never happen
        if (waveformCycleStep >= WAVEFORM_LENGTH) {
            return;
        }

        const uint8_t waveform = pgm_read_byte(&(WAVEFORMS[activeWaveform]));
        const bool wfBit = static_cast<uint8_t>(waveform >> waveformCycleStep) & 0b1u;
        blinkerPin.set(wfBit);

        waveformCycleStep++;
        const uint8_t stepDivisions = waveformStepDivisions();
        const uint8_t nextWaveformStepDivisions = ACCESS_BYTE(OCR0B) + (stepDivisions > 0 ? stepDivisions : 1);
        const bool hasRemainingWaveformBits = waveformCycleStep < WAVEFORM_LENGTH;
        const bool hasRemainingDivisions = nextWaveformStepDivisions < ACCESS_BYTE(OCR0A);
        if (hasRemainingWaveformBits && hasRemainingDivisions) {
            ACCESS_BYTE(OCR0B) = nextWaveformStepDivisions;
        }
    }

    void onWaveformCycleEnd() {
        blinkerPin.clear();

        waveformCycleStep = 0;
        ACCESS_BYTE(OCR0B) = waveformStepDivisions();
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

    // enable compa & compb interrupts
    ACCESS_BYTE(TIMSK0) |= BIT_MASK(OCIE0A) | BIT_MASK(OCIE0B);

    // set pre-scaler to 1024 and start timer
    ACCESS_BYTE(TCCR0B) |= BIT_MASK(CS02) | BIT_MASK(CS00);

    // ----------------

    sei();

    mainCore.run();
}

// ----------------
