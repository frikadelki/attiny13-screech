
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

uint8_t __builtin_avr_swap(uint8_t);

// ----------------

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

inline __attribute__((always_inline))
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

        0b11110000, // -- square

        0b10111000, // -- sawtooth

        0b01110101, // -- triangle
};

const uint8_t WAVEFORMS_COUNT = sizeof(WAVEFORMS) / sizeof(uint8_t);

inline __attribute__((always_inline))
uint8_t readWaveform(const uint8_t waveformIndex) {
    divv::div(waveformIndex, WAVEFORMS_COUNT);
    const uint8_t indexMod = divv::remainder;
    return pgm_read_byte(&(WAVEFORMS[indexMod]));
}

// ----------------

// -------- WAVEFORM GEN --------

namespace WaveformGen {
    typedef struct NoteInfo {
        uint8_t noteDivisions;

        uint8_t waveform;
    } NoteInfo;

    inline __attribute__((always_inline))
    extern NoteInfo nextNoteSource();

    void restartGenerator();
}

namespace WaveformGen {
    namespace {
        const uint16_t TIMER_PRESCALER = 1024;

        const uint16_t TIMER_COUNTS_IN_SECOND = F_CPU / TIMER_PRESCALER;

        const uint16_t TIMER_COUNTS_PER_BEAT = TIMER_COUNTS_IN_SECOND / 8;

        typedef OutputPin<DDRB, PORTB, PINB, 0> blinkerPin;

        struct WaveformGeneratorState {
            uint16_t timeCounter = 0;

            WaveformGen::NoteInfo activeNote = { 0, 0 };

            uint8_t waveformStepDivisions = 0;

            uint8_t liveWaveform = 0;
        };

        WaveformGeneratorState wgs;

        // this is weird, compilation produces different code
        // depending if data is "packed" or "flat
        // seems to be affected by members in the struct too
        /*
        uint16_t _timeCounter = 0;

        WaveformGen::NoteInfo _activeNote = { 0, 0 };

        uint8_t _liveWaveform = 0;

        uint8_t _waveformStepDivisions = 0;
        */

        inline __attribute__((always_inline))
        uint16_t& timeCounter() {
            //return _timeCounter;
            return wgs.timeCounter;
        }

        inline __attribute__((always_inline))
        NoteInfo& activeNote() {
            //return _activeNote;
            return wgs.activeNote;
        }

        inline __attribute__((always_inline))
        uint8_t& liveWaveform() {
            //return _liveWaveform;
            return wgs.liveWaveform;
        }

        inline __attribute__((always_inline))
        uint8_t& waveformStepDivisions() {
            //return _waveformStepDivisions;
            return wgs.waveformStepDivisions;
        }

        inline __attribute__((always_inline))
        void onWaveStep() {
            const bool wfBit = liveWaveform() & 0b1u;
            liveWaveform() >>= 1u;
            blinkerPin::set(wfBit);
            ACCESS_BYTE(OCR0B) += waveformStepDivisions();
        }

        inline __attribute__((always_inline))
        void fetchNextNote() {
            activeNote() = nextNoteSource();
            waveformStepDivisions() = divv::div(activeNote().noteDivisions, WAVEFORM_LENGTH + 1);
        }

        inline __attribute__((always_inline))
        void primeTimers() {
            ACCESS_BYTE(OCR0A) = activeNote().noteDivisions;
            ACCESS_BYTE(OCR0B) = waveformStepDivisions();
        }

        inline __attribute__((always_inline))
        void primeNextWavePeriod() {
            blinkerPin::clear();
            liveWaveform() = activeNote().waveform;
            primeTimers();
        }

        inline __attribute__((always_inline))
        void onWavePeriodEnd() {
            timeCounter() += ACCESS_BYTE(OCR0A);
            if (timeCounter() >= TIMER_COUNTS_PER_BEAT) {
                timeCounter() = 0;
                fetchNextNote();
            }
            primeNextWavePeriod();
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-attributes"
        ISR(TIM0_COMPB_vect) {
            onWaveStep();
        }

        ISR(TIM0_COMPA_vect) {
            onWavePeriodEnd();
        }
#pragma clang diagnostic pop
    }

    inline __attribute__((always_inline))
    void restartGenerator() {
        cli();

        blinkerPin::init();

        // set timer counter mode to CTC
        ACCESS_BYTE(TCCR0A) |= BIT_MASK(WGM01);

        // enable compa & compb interrupts
        ACCESS_BYTE(TIMSK0) |= BIT_MASK(OCIE0A) | BIT_MASK(OCIE0B);

        timeCounter() = TIMER_COUNTS_PER_BEAT;

        // set pre-scaler to 1024 and start timer
        ACCESS_BYTE(TCCR0B) |= BIT_MASK(CS02) | BIT_MASK(CS00);

        sei();
    }
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

    static const uint8_t INPUT_FILTER_MASK = 0b1111111u;

    static const uint8_t INPUT_FILTER_MAX = INPUT_FILTER_MASK;

    // 7 msb bits are filter value
    // 0th bit indicates if rising edge was detected during the latest poll
    static uint8_t buttonsState[];

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

    __attribute__((noinline))
    static void filterButton(const bool isOn, const InputBtn btn) {
        uint8_t btnFilter = static_cast<uint8_t>(buttonsState[btn] >> 1u) & INPUT_FILTER_MASK;
        bool edge = false;
        filter(isOn, btnFilter, edge);
        buttonsState[btn] = static_cast<uint8_t>(btnFilter << 1u) | edge;
    }

public:
    inline __attribute__((always_inline))
    static void init() {
        // we can skip this and save some instructions because
        // micro's startup state matches defaults for ComboPin
        // inputMode::init();
        // inputMinus::init();
        // inputClick::init();
        // inputPlus::init();
    }

    inline __attribute__((always_inline))
    static void pollInputs() {
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
    static bool isRisingEdge(const InputBtn btn) {
        return (buttonsState[btn] & 0b1u);
    }

    inline __attribute__((always_inline))
    static void setLEDs(const bool i3, const bool i2, const bool i1, const bool i0) {
        inputMode::outputSet(i3);
        inputMinus::outputSet(i2);
        inputClick::outputSet(i1);
        inputPlus::outputSet(i0);
    }

    inline __attribute__((always_inline))
    static void setLEDs(const uint8_t val) {
        inputPlus::outputSet(val & 0b0001u);
        inputClick::outputSet(val & 0b0010u);
        inputMinus::outputSet(val & 0b0100u);
        inputMode::outputSet(val & 0b1000u);
    }
};

uint8_t InputHandler::buttonsState[InputButtonsCount] = { 0, 0, 0, 0 };

// ----------------

// -------- NOTES SEQUENCES --------

namespace ActiveNoteNotesSequence {
    namespace {
        uint8_t activeNoteIndex = 0;

        uint8_t  activeWaveformIndex = 0;
    }

    class Logic {
    public:
        inline __attribute__((always_inline))
        static void init() {
        }

        inline __attribute__((always_inline))
        static void onCycle() {
            if (InputHandler::isRisingEdge(InputBtnMode)) {
                activeNoteIndex--;
            }
            if (InputHandler::isRisingEdge(InputBtnMinus)) {
                activeNoteIndex++;
            }
            if (InputHandler::isRisingEdge(InputBtnClick)) {
                activeWaveformIndex--;
            }
            if (InputHandler::isRisingEdge(InputBtnPlus)) {
                activeWaveformIndex++;
            }
            InputHandler::setLEDs(true, false, true, false);
        }

        inline __attribute__((always_inline))
        static WaveformGen::NoteInfo nextNote() {
            const uint8_t noteDivisions = readNoteDivisions(activeNoteIndex);
            const uint8_t noteWaveform = readWaveform(activeWaveformIndex);
            return WaveformGen::NoteInfo { noteDivisions, noteWaveform };
        }
    };
}

namespace AutoNotesSequence {
    namespace {
        uint8_t activeNoteIndex = 0;

        uint8_t  activeWaveformIndex = 0;
    }

    class Logic {
    public:
        inline __attribute__((always_inline))
        static void init() {
        }

        inline __attribute__((always_inline))
        static void onCycle() {
            if (InputHandler::isRisingEdge(InputBtnMode)) {
                advanceWaveform();
            }
            if (InputHandler::isRisingEdge(InputBtnMinus)) {
                advanceWaveform();
            }
            if (InputHandler::isRisingEdge(InputBtnClick)) {
                advanceWaveform();
            }
            if (InputHandler::isRisingEdge(InputBtnPlus)) {
                advanceWaveform();
            }
            InputHandler::setLEDs(true, false, true, false);
        }

        inline __attribute__((always_inline))
        static WaveformGen::NoteInfo nextNote() {
            const uint8_t noteDivisions = readNoteDivisions(activeNoteIndex++);
            const uint8_t noteWaveform = readWaveform(activeWaveformIndex);
            return WaveformGen::NoteInfo { noteDivisions, noteWaveform };
        }

    private:
        static void advanceWaveform() {
            activeWaveformIndex++;
        }
    };
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

        uint8_t activeWaveformIndex = 0;
    }

    class Logic {
    public:
        inline __attribute__((always_inline))
        static void init() {
        }

        inline __attribute__((always_inline))
        static void onCycle() {
            if (InputHandler::isRisingEdge(InputBtnMode)) {
                melodyNoteIndex = 0;
            }
            if (InputHandler::isRisingEdge(InputBtnMinus)) {
            }
            if (InputHandler::isRisingEdge(InputBtnClick)) {
                activeWaveformIndex--;
            }
            if (InputHandler::isRisingEdge(InputBtnPlus)) {
                activeWaveformIndex++;
            }
            InputHandler::setLEDs(true, false, true, false);
        }

        inline __attribute__((always_inline))
        static WaveformGen::NoteInfo nextNote() {
            uint8_t point = readMelodyPoint(melodyNoteIndex / 2);
            if (0 == melodyNoteIndex % 2) {
                point = __builtin_avr_swap(point);
            }
            const uint8_t noteDivisionsIndex = point & 0b1111u;
            melodyNoteIndex++; // funny thing, moving this line up or down increases code size
            const uint8_t noteDivisions = readNoteDivisions(noteDivisionsIndex);
            const uint8_t noteWaveform = noteDivisionsIndex > 0 ? readWaveform(activeWaveformIndex) : 0;
            return WaveformGen::NoteInfo { noteDivisions, noteWaveform };
        }
    };
}

namespace Fooz {
    namespace {
        uint8_t activeNoteIndex = 0;

        uint8_t activeWaveformIndex = 0;
    }

    class Logic {
    public:
        inline __attribute__((always_inline))
        static void init() {
        }

        inline __attribute__((always_inline))
        static void onCycle() {
            if (InputHandler::isRisingEdge(InputBtnMode)) {
                activeNoteIndex--;
            }
            if (InputHandler::isRisingEdge(InputBtnMinus)) {
                activeNoteIndex++;
            }
            if (InputHandler::isRisingEdge(InputBtnClick)) {
                activeWaveformIndex--;
            }
            if (InputHandler::isRisingEdge(InputBtnPlus)) {
                activeWaveformIndex++;
            }
            InputHandler::setLEDs(activeNoteIndex);
        }

        inline __attribute__((always_inline))
        static WaveformGen::NoteInfo nextNote() {
            const uint8_t noteDivisions = readNoteDivisions(activeNoteIndex);
            const uint8_t noteWaveform = readWaveform(activeWaveformIndex);
            return WaveformGen::NoteInfo { noteDivisions, noteWaveform };
        }
    };
}

// ----------------

// -------- MAIN --------

namespace MainProtos {
    class EmptyMainLogic {
    public:
        inline __attribute__((always_inline))
        static void init() {
        }

        inline __attribute__((always_inline))
        static void onCycle() {
            if (InputHandler::isRisingEdge(InputBtnMode)) {
            }
            if (InputHandler::isRisingEdge(InputBtnMinus)) {
            }
            if (InputHandler::isRisingEdge(InputBtnClick)) {
            }
            if (InputHandler::isRisingEdge(InputBtnPlus)) {
            }
            InputHandler::setLEDs(true, false, true, false);
        }

        static WaveformGen::NoteInfo nextNote() {
            return WaveformGen::NoteInfo { 0, 0 };
        }
    };
}

typedef FlashMemoryMelody::Logic MainLogic;

WaveformGen::NoteInfo WaveformGen::nextNoteSource() {
    return MainLogic::nextNote();
}

class Main {
public:
    inline __attribute__((always_inline))
    static void init() {
        InputHandler::init();
        MainLogic::init();
        WaveformGen::restartGenerator();
    }

    inline __attribute__((always_inline))
    static void run() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
        while (true) {
            cycle();
        }
#pragma clang diagnostic pop
    }

private:
    inline __attribute__((always_inline))
    static void cycle() {
        InputHandler::pollInputs();
        MainLogic::onCycle();
    }
};

int main() {
    Main::init();
    Main::run();
    return 0;
}

// ----------------
