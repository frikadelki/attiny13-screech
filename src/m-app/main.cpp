#include "../m-toolbox/Macro.h"
#include "../m-toolbox/InputPin.h"
#include "../m-toolbox/OutputPin.h"

#include <util/delay.h>
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

class NotesSource {
private:
    uint8_t activeNoteIndex = 0;

public:
    void nextNote() {
        activeNoteIndex = (activeNoteIndex + 1) % NOTES_COUNT;
    }

    uint8_t activeNoteDivisions() {
        return pgm_read_byte(&(NOTES_DIVISIONS[activeNoteIndex]));
    }
};

// ----------------

// -------- WAVEFORM DATA --------

const uint8_t WAVEFORM_LENGTH = 8u;

const uint8_t WAVEFORMS[] PROGMEM = {
        0b00000000,

        0b10000000,
        0b11110000,
        0b11111100,

        0b10001000,
        0b11001100,

        0b11110010,

        0b10010010,
        0b10101010,
};

const uint8_t WAVEFORMS_COUNT = sizeof(WAVEFORMS) / sizeof(uint8_t);

class WaveformsSource {
private:
    uint8_t activeWaveformIndex = 0;

public:
    void nextWaveform() {
        activeWaveformIndex = (activeWaveformIndex + 1) % WAVEFORMS_COUNT;
    }

    uint8_t activeWaveform() {
        return pgm_read_byte(&(WAVEFORMS[activeWaveformIndex]));
    }
};

// ----------------

// -------- WAVEFORM GEN --------

// active note divisions stored in OCR0A
class WaveformGenerator {
private:
    OutputPin<DDRB, PORTB, PINB, 0> blinkerPin;

    NotesSource notesSource;

    WaveformsSource waveformsSource;

    uint8_t waveformStep = 0;

    uint8_t waveformStepDivisions() {
        return ACCESS_BYTE(OCR0A) / (WAVEFORM_LENGTH + 1);
    }

public:
    WaveformGenerator() {
        blinkerPin.clear();

        // -------- configuring timer --------

        // set timer counter mode to CTC
        SET_BYTE_BIT(TCCR0A, WGM01);

        // enable compa & compb interrupts
        ACCESS_BYTE(TIMSK0) |= BIT_MASK(OCIE0A) | BIT_MASK(OCIE0B);

        // set pre-scaler to 1024 and start timer
        ACCESS_BYTE(TCCR0B) |= BIT_MASK(CS02) | BIT_MASK(CS00);

        // ----------------

        updateNote();
    }

    void nextNote() {
        notesSource.nextNote();
        updateNote();
    }

    void nextWaveform() {
        waveformsSource.nextWaveform();
    }

    void updateNote() {
        cli();

        uint8_t divisions = notesSource.activeNoteDivisions();
        divisions = divisions > 0 ? divisions : 1;
        ACCESS_BYTE(OCR0A) = divisions;

        onEnd();

        // clear any pending timer interrupts and restart timer fom zero
        ACCESS_BYTE(TIFR0) = BIT_MASK(OCF0B) | BIT_MASK(OCF0A) | BIT_MASK(TOV0);
        ACCESS_BYTE(TCNT0) = 0;

        sei();
    }

    void onStep() {
        // this actually should never happen
        if (waveformStep >= WAVEFORM_LENGTH) {
            return;
        }

        const uint8_t waveform = waveformsSource.activeWaveform();
        const bool wfBit = static_cast<uint8_t>(waveform >> waveformStep) & 0b1u;
        blinkerPin.set(wfBit);

        waveformStep++;
        const uint8_t stepDivisions = waveformStepDivisions();
        const uint8_t nextWaveformStepDivisions = ACCESS_BYTE(OCR0B) + (stepDivisions > 0 ? stepDivisions : 1);
        const bool hasRemainingWaveformBits = waveformStep < WAVEFORM_LENGTH;
        const bool hasRemainingDivisions = nextWaveformStepDivisions < ACCESS_BYTE(OCR0A);
        if (hasRemainingWaveformBits && hasRemainingDivisions) {
            ACCESS_BYTE(OCR0B) = nextWaveformStepDivisions;
        }
    }

    void onEnd() {
        blinkerPin.clear();

        waveformStep = 0;
        ACCESS_BYTE(OCR0B) = waveformStepDivisions();
    }
};

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

    static void filter(const bool isOn, uint8_t& value, bool& edge) {
        if (isOn) {
            if (value < INPUT_FILTER_MAX) {
                value++;
                if (value == INPUT_FILTER_MAX) {
                    edge = true;
                }
            }
        } else {
            if (value > 0) {
                value--;
            }
        }
    }

public:
    InputHandler() = default;

    void readAndFilterInput() {
        inputRisingEdge[InputBtnL] = false;
        inputRisingEdge[InputBtnR] = false;

        uint8_t filterL = inputFilter >> INPUT_FILTER_NIBBLE_SIZE;
        filter(inputL.read(), filterL, inputRisingEdge[InputBtnL]);

        uint8_t filterR = inputFilter & INPUT_FILTER_MAX;
        filter(inputR.read(), filterR, inputRisingEdge[InputBtnR]);

        inputFilter = static_cast<uint8_t>(filterL << INPUT_FILTER_NIBBLE_SIZE) | filterR;
    }

    bool isRisingEdge(const InputBtn button) {
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
    InputHandler inputHandler;

    WaveformGenerator waveformGenerator;

    void cycle() {
        inputHandler.readAndFilterInput();

        if (inputHandler.isRisingEdge(InputBtnL)) {
            waveformGenerator.nextNote();
        }
        if (inputHandler.isRisingEdge(InputBtnR)) {
            waveformGenerator.nextWaveform();
        }

        _delay_ms(7);
    }

public:
    Main() = default;

    void run() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
        while (true) {
            cycle();
        }
#pragma clang diagnostic pop
    }

    void onWaveformCycleStep() {
        waveformGenerator.onStep();
    }

    void onWaveformCycleEnd() {
        waveformGenerator.onEnd();
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

    sei();

    mainCore.run();
}

// ----------------
