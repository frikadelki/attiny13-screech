
#include "../m-toolbox/Macro.h"
#include "../m-toolbox/ComboPin.h"
#include "../m-toolbox/OutputPin.h"

#include <util/delay.h>
#include <avr/pgmspace.h>

// -------- UTILS --------

namespace divv {
    uint8_t remainder;

    uint8_t div(uint8_t dividend, const uint8_t divisor) {
        uint8_t result = 0;
        while (dividend >= divisor) {
            dividend -= divisor;
            result++;
        }
        remainder = dividend;
        return result;
    }
}

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
    divv::div(noteIndex, NOTES_COUNT);
    const uint8_t indexMod = divv::remainder;
    return pgm_read_byte(&(NOTES_DIVISIONS[indexMod]));
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
    divv::div(waveformIndex, WAVEFORMS_COUNT);
    const uint8_t indexMod = divv::remainder;
    return pgm_read_byte(&(WAVEFORMS[indexMod]));
}

// ----------------

// -------- WAVEFORM GEN --------

class WaveformGen {
public:
    typedef struct NoteInfo {
        uint8_t noteDivisions;
        uint8_t waveform;
    } NoteInfo;

    typedef NoteInfo (*NotesSequence)();

private:
    static const uint16_t TIMER_PRESCALER = 1024;

    static const uint16_t TIMER_COUNTS_IN_SECOND = F_CPU / TIMER_PRESCALER;

    static const uint16_t TIMER_COUNTS_PER_BEAT = TIMER_COUNTS_IN_SECOND / 8;

    struct WaveformGeneratorState {
        NotesSequence notesSequence = nullptr;

        uint16_t timeCounter = 0;

        NoteInfo activeNote = { 0, 0 };

        uint8_t waveformStep = 0;
    };

    typedef OutputPin<DDRB, PORTB, PINB, 0> blinkerPin;

    static WaveformGeneratorState wgs;

    inline __attribute__((always_inline))
    static uint8_t waveformStepDivisions() {
        return divv::div(wgs.activeNote.noteDivisions, WAVEFORM_LENGTH + 1);
    }

    inline __attribute__((always_inline))
    static void primeTimers() {
        ACCESS_BYTE(OCR0A) = wgs.activeNote.noteDivisions;
        ACCESS_BYTE(OCR0B) = waveformStepDivisions();
    }

    static void primeNextWavePeriod() {
        blinkerPin::clear();
        wgs.waveformStep = 0;
        primeTimers();
    }

public:
    inline __attribute__((always_inline))
    static void restartGenerator(NotesSequence newNotesSequence) {
        cli();

        blinkerPin::init();

        // set timer counter mode to CTC
        ACCESS_BYTE(TCCR0A) |= BIT_MASK(WGM01);

        // enable compa & compb interrupts
        ACCESS_BYTE(TIMSK0) |= BIT_MASK(OCIE0A) | BIT_MASK(OCIE0B);

        // clear any relevant pending interrupts and reset timer counter
        ACCESS_BYTE(TIFR0) = BIT_MASK(OCF0B) | BIT_MASK(OCF0A) | BIT_MASK(TOV0);
        ACCESS_BYTE(TCNT0) = 0;

        wgs.notesSequence = newNotesSequence;
        wgs.timeCounter = 0;
        wgs.activeNote = wgs.notesSequence();
        primeNextWavePeriod();

        // set pre-scaler to 1024 and start timer
        ACCESS_BYTE(TCCR0B) |= BIT_MASK(CS02) | BIT_MASK(CS00);

        sei();
    }

    inline __attribute__((always_inline))
    static void onWaveStep() {
        const bool wfBit = static_cast<uint8_t>(wgs.activeNote.waveform >> wgs.waveformStep) & 0b1u;
        blinkerPin::set(wfBit);

        wgs.waveformStep++;
        const uint8_t stepDivisions = waveformStepDivisions();
        const uint8_t nextWaveformStepDivisions = ACCESS_BYTE(OCR0B) + (stepDivisions > 0 ? stepDivisions : 1);
        const bool hasRemainingWaveformBits = wgs.waveformStep < WAVEFORM_LENGTH;
        const bool hasRemainingDivisions = nextWaveformStepDivisions < wgs.activeNote.noteDivisions;
        if (hasRemainingWaveformBits && hasRemainingDivisions) {
            ACCESS_BYTE(OCR0B) = nextWaveformStepDivisions;
        }
    }

    inline __attribute__((always_inline))
    static void onWavePeriodEnd() {
        wgs.timeCounter += ACCESS_BYTE(OCR0A);
        if (wgs.timeCounter >= TIMER_COUNTS_PER_BEAT) {
            wgs.timeCounter -= TIMER_COUNTS_PER_BEAT;
            wgs.activeNote = wgs.notesSequence();
        }
        primeNextWavePeriod();
    }
};

WaveformGen::WaveformGeneratorState WaveformGen::wgs;

ISR(TIM0_COMPB_vect) {
    WaveformGen::onWaveStep();
}

ISR(TIM0_COMPA_vect) {
    WaveformGen::onWavePeriodEnd();
}

// ----------------

// -------- INPUT HANDLER --------

typedef enum InputBtn {
    InputButtonsCount =  4,

    InputBtnMode =  3,
    InputBtnMinus = 2,
    InputBtnClick = 1,
    InputBtnPlus =  0,
} InputBtn;

class InputHandler {
private:
    typedef ComboPin<DDRB, PORTB, PINB, 4> inputMode;

    typedef ComboPin<DDRB, PORTB, PINB, 3> inputMinus;

    typedef ComboPin<DDRB, PORTB, PINB, 2> inputClick;

    typedef ComboPin<DDRB, PORTB, PINB, 1> inputPlus;

    static const uint8_t INPUT_FILTER_MASK = 0b1111u;

    static const uint8_t INPUT_FILTER_MAX = INPUT_FILTER_MASK;

    // 7 msb bits are filter value
    // 0th bit indicates if rising edge was detected during the latest poll
    uint8_t buttonsState[InputButtonsCount] = { 0, 0, 0, 0 };

    inline __attribute__((always_inline))
    static void filter(const bool isOn, uint8_t& value, bool& edge) {
        if (isOn) {
            if (value < INPUT_FILTER_MAX) {
                value++;
                if (value == INPUT_FILTER_MAX) {
                    edge = true;
                }
            }
        } else {
            value = 0;
        }
    }

    void filterButton(const bool isOn, const InputBtn btn) {
        uint8_t btnFilter = static_cast<uint8_t>(buttonsState[btn] >> 1u) & INPUT_FILTER_MASK;
        bool edge = false;
        filter(isOn, btnFilter, edge);
        buttonsState[btn] = static_cast<uint8_t>(btnFilter << 1u) | edge;
    }

public:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-equals-default"
    InputHandler() { // NOLINT(hicpp-use-equals-default)
        // we can skip this and save some instructions because
        // micro's startup state matches defaults for ComboPin
        // inputMode::init();
        // inputMinus::init();
        // inputClick::init();
        // inputPlus::init();
    };
#pragma clang diagnostic pop

    inline __attribute__((always_inline))
    void pollInputs() {
        inputMode::inputPrime();
        inputMinus::inputPrime();
        inputClick::inputPrime();
        inputPlus::inputPrime();
        comboPinWaitInputSettle();
        filterButton(inputMode::inputRead(), InputBtnMode);
        filterButton(inputMinus::inputRead(), InputBtnMinus);
        filterButton(inputClick::inputRead(), InputBtnClick);
        filterButton(inputPlus::inputRead(), InputBtnPlus);
    }

    inline __attribute__((always_inline))
    bool isRisingEdge(const InputBtn btn) {
        return (buttonsState[btn] & 0b1u);
    }

    inline __attribute__((always_inline))
    void setLEDs(const bool i3, const bool i2, const bool i1, const bool i0) {
        inputMode::outputSet(i3);
        inputMinus::outputSet(i2);
        inputClick::outputSet(i1);
        inputPlus::outputSet(i0);
    }

    void setLEDs(uint8_t val) {
        setLEDs(
                static_cast<uint8_t>(val >> 3u) & 1u,
                static_cast<uint8_t>(val >> 2u) & 1u,
                static_cast<uint8_t>(val >> 1u) & 1u,
                static_cast<uint8_t>(val >> 0u) & 1u);
    }
};

// ----------------

// -------- MAIN --------

namespace ActiveNoteNotesSequence {
    namespace {
        uint8_t activeNoteIndex = 0;

        uint8_t  activeWaveformIndex = 0;
    }

    void advanceNote() {
        activeNoteIndex++;
    }

    void advanceWaveform() {
        activeWaveformIndex++;
    }

    WaveformGen::NoteInfo nextNote() {
        const uint8_t noteDivisions = readNoteDivisions(activeNoteIndex);
        const uint8_t noteWaveform = readWaveform(activeWaveformIndex);
        return WaveformGen::NoteInfo { noteDivisions, noteWaveform };
    }
}

namespace AutoNotesSequence {
    namespace {
        uint8_t activeNoteIndex = 0;

        uint8_t  activeWaveformIndex = 0;
    }

    void advanceWaveform() {
        activeWaveformIndex++;
    }

    WaveformGen::NoteInfo nextNote() {
        const uint8_t noteDivisions = readNoteDivisions(activeNoteIndex++);
        const uint8_t noteWaveform = readWaveform(activeWaveformIndex);
        return WaveformGen::NoteInfo { noteDivisions, noteWaveform };
    }
};

namespace FlashMemoryMelody {
    namespace {
        const uint8_t _SAMPLE_MELODY_0_POINTS[] PROGMEM = {
            0xAA, 0x0A, 0x08, 0xA0,
            0xC0, 0x00, 0x50, 0x00,
            0x80, 0x05, 0x00, 0x30,
            0x06, 0x07, 0x06, 0x60,

            0x00, 0x00, 0x00, 0x00,
        };

        const uint8_t _SAMPLE_MELODY_0_POINTS_COUNT = sizeof(_SAMPLE_MELODY_0_POINTS) / sizeof(uint8_t);

        // one point is two 4-bit notes indices in NOTES_DIVISIONS array
        const uint8_t _SAMPLE_MELODY_0_NOTES_COUNT = _SAMPLE_MELODY_0_POINTS_COUNT * 2;

        uint8_t readMelodyPoint(const uint8_t pointIndex) {
            divv::div(pointIndex, _SAMPLE_MELODY_0_POINTS_COUNT);
            const uint8_t indexMod = divv::remainder;
            return pgm_read_byte(&(_SAMPLE_MELODY_0_POINTS[indexMod]));
        }
    }

    namespace {
        uint8_t melodyNoteIndex = 0;

        uint8_t  activeWaveformIndex = 0;
    }

    void advanceWaveform() {
        activeWaveformIndex++;
    }

    WaveformGen::NoteInfo nextNote() {
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
        return WaveformGen::NoteInfo { noteDivisions, noteWaveform };
    }
}

class Main {
private:
    InputHandler inputHandler;

public:
    Main() {
        WaveformGen::restartGenerator(&FlashMemoryMelody::nextNote);
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
        inputHandler.pollInputs();
        if (inputHandler.isRisingEdge(InputBtnMode)) {
            FlashMemoryMelody::advanceWaveform();
        }
        if (inputHandler.isRisingEdge(InputBtnMinus)) {
            FlashMemoryMelody::advanceWaveform();
        }
        if (inputHandler.isRisingEdge(InputBtnClick)) {
            FlashMemoryMelody::advanceWaveform();
        }
        if (inputHandler.isRisingEdge(InputBtnPlus)) {
            FlashMemoryMelody::advanceWaveform();
        }

        inputHandler.setLEDs(true, false, true, false);
    }
};

// ----------------

// -------- INIT --------

Main mainCore;

int main() {
    mainCore.run();
}

// ----------------
