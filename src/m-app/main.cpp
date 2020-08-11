#include "../m-toolbox/Macro.h"
#include "../m-toolbox/InputPin.h"
#include "../m-toolbox/OutputPin.h"

#include <util/delay.h>
#include <avr/pgmspace.h>

// -------- NOTES DATA --------

const uint8_t NOTES_DIVISIONS[] PROGMEM = {
        255,
        200, 		//C6    //0x1
        193, 		//D6    //0x2
        185, 		//E6    //0x3
        181, 		//F6    //0x4
        171, 		//G6    //0x5
        209, 		//A6    //0x6
        203, 		//B6    //0x7
        143, 		//C7    //0x8
        129, 		//D7    //0x9
        113, 		//E7    //0xA
        104, 		//F7    //0xB
        86, 		//G7    //0xC
        161, 		//A7    //0xD
        149, 		//B7    //0xE
        255
};

const uint8_t NOTES_COUNT = sizeof(NOTES_DIVISIONS) / sizeof(uint8_t);

uint8_t readNoteDivisions(const uint8_t noteIndex) {
    return pgm_read_byte(&(NOTES_DIVISIONS[noteIndex % NOTES_COUNT]));
}

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

uint8_t readWaveform(const uint8_t waveformIndex) {
    return pgm_read_byte(&(WAVEFORMS[waveformIndex % WAVEFORMS_COUNT]));
}

// ----------------

// -------- WAVEFORM GEN --------

typedef struct NoteInfo {
    const uint8_t noteDivisions;
    const uint8_t waveform;
} NoteInfo;

class NotesSequence {
public:
    virtual NoteInfo nextNote() = 0;
};

const uint16_t TIMER_COUNTS_IN_SECOND = 9500;

const uint16_t TIMER_COUNTS_PER_BEAT = TIMER_COUNTS_IN_SECOND / 8;

// active note divisions stored in OCR0A
class WaveformGenerator {
private:
    OutputPin<DDRB, PORTB, PINB, 0> blinkerPin;

    uint8_t waveform = 0;

    uint8_t waveformStep = 0;

    uint8_t nextNoteDivisions = 255;

    uint8_t nextNoteWaveform = 0;

    NotesSequence &notesSequence;

    uint16_t timeCounter = 0;

public:
    explicit WaveformGenerator(NotesSequence &notesSequence) :
        notesSequence(notesSequence) {
        cli();

        blinkerPin.clear();

        // set timer counter mode to CTC
        SET_BYTE_BIT(TCCR0A, WGM01);

        // enable compa & compb interrupts
        ACCESS_BYTE(TIMSK0) |= BIT_MASK(OCIE0A) | BIT_MASK(OCIE0B);

        // load default interrupt divisions from 'next*' fields
        primeTimerDivisions();

        // set pre-scaler to 1024 and start timer
        ACCESS_BYTE(TCCR0B) |= BIT_MASK(CS02) | BIT_MASK(CS00);

        sei();
    }

// these are only public because they are interrupt callbacks
// maybe we can hide them somehow, we are monopolizing timer after all
public:
    void onStep() {
        // this actually should never happen
        if (waveformStep >= WAVEFORM_LENGTH) {
            return;
        }

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
        timeCounter += ACCESS_BYTE(OCR0A);
        if (timeCounter >= TIMER_COUNTS_PER_BEAT) {
            timeCounter -= TIMER_COUNTS_PER_BEAT;

            NoteInfo note = notesSequence.nextNote();
            nextNoteDivisions = note.noteDivisions;
            nextNoteWaveform = note.waveform;
        }

        blinkerPin.clear();

        waveformStep = 0;
        waveform = nextNoteWaveform;

        primeTimerDivisions();
    }

private:
    void primeTimerDivisions() {
        ACCESS_BYTE(OCR0A) = nextNoteDivisions;
        ACCESS_BYTE(OCR0B) = waveformStepDivisions();
    }

    uint8_t waveformStepDivisions() {
        return ACCESS_BYTE(OCR0A) / (WAVEFORM_LENGTH + 1);
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

class ActiveNoteNotesSequence : public NotesSequence {
private:
    uint8_t activeNoteIndex = 0;

    uint8_t  activeWaveformIndex = 0;

public:
    void advanceNote() {
        activeNoteIndex++;
    }

    void advanceWaveform() {
        activeWaveformIndex++;
    }

    NoteInfo nextNote() final {
        const uint8_t noteDivisions = readNoteDivisions(activeNoteIndex);
        const uint8_t noteWaveform = readWaveform(activeWaveformIndex);
        return NoteInfo { noteDivisions, noteWaveform };
    }
};

class AutoNotesSequence : public NotesSequence {
    uint8_t activeNoteIndex = 0;

    uint8_t  activeWaveformIndex = 0;

public:
    void advanceWaveform() {
        activeWaveformIndex++;
    }

    NoteInfo nextNote() final {
        const uint8_t noteDivisions = readNoteDivisions(activeNoteIndex++);
        const uint8_t noteWaveform = readWaveform(activeWaveformIndex);
        return NoteInfo { noteDivisions, noteWaveform };
    }
};

const uint8_t _SAMPLE_MELODY_0_POINTS[] PROGMEM = {
        0xAA, 0x0A, 0x08, 0xA0,
        0xC0, 0x00, 0x50, 0x00,
        0x80, 0x05, 0x00, 0x30,
        0x06, 0x07, 0x06, 0x60,

        0x00, 0x00, 0x00, 0x00,
};

const uint8_t _SAMPLE_MELODY_0_POINTS_COUNT = sizeof(_SAMPLE_MELODY_0_POINTS) / sizeof(uint8_t);

// 1 point is 2 4-bit notes indices in NOTES_DIVISIONS array
const uint8_t _SAMPLE_MELODY_0_NOTES_COUNT = _SAMPLE_MELODY_0_POINTS_COUNT * 2;

uint8_t readMelodyPoint(const uint8_t pointIndex) {
    return pgm_read_byte(&(_SAMPLE_MELODY_0_POINTS[pointIndex % _SAMPLE_MELODY_0_POINTS_COUNT]));
}

class PainMemoryNotesSequence : public NotesSequence {
    uint8_t melodyNoteIndex = 0;

    uint8_t  activeWaveformIndex = 0;

public:
    void advanceWaveform() {
        activeWaveformIndex++;
    }

    NoteInfo nextNote() final {
        const uint8_t point = readMelodyPoint(melodyNoteIndex / 2);
        uint8_t noteDivisionsIndex;
        if (0 == melodyNoteIndex % 2) {
            noteDivisionsIndex = (point >> 4u);
        } else {
            noteDivisionsIndex = point & 0b1111u;
        }
        melodyNoteIndex++;

        const uint8_t noteDivisions = readNoteDivisions(noteDivisionsIndex);
        const uint8_t noteWaveform = noteDivisionsIndex > 0 ? readWaveform(activeWaveformIndex) : 0;
        return NoteInfo { noteDivisions, noteWaveform };
    }
};

class Main {
private:
    ActiveNoteNotesSequence notesSequence;

    WaveformGenerator waveformGenerator;

    InputHandler inputHandler;

public:
    Main() : waveformGenerator(notesSequence) {
    }

    void run() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
        while (true) {
            cycle();
        }
#pragma clang diagnostic pop
    }

private:
    void cycle() {
        inputHandler.readAndFilterInput();

        if (inputHandler.isRisingEdge(InputBtnL)) {
            notesSequence.advanceNote();
        }
        if (inputHandler.isRisingEdge(InputBtnR)) {
            notesSequence.advanceWaveform();
        }

        _delay_ms(7);
    }

// interrupt callbacks
public:
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
    static Main mainCore;
    _mainCore = &mainCore;

    mainCore.run();
}

// ----------------
