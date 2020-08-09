// -------- NOTES --------
/*
A0 xx
B0 xx
C0 16
D0 18
E0 21
F0 22
G0 25
A1 28
B1 31
C1 33
D1 37
E1 41
F1 44
G1 49
A2 55
B2 62
C2 65
D2 73
E2 82
F2 87
G2 98
A3 110
B3 123
C3 131
D3 147
E3 165
F3 175
G3 196
A4 220
B4 247
C4 262
D4 294
E4 330
F4 349
G4 392
A5 440
B5 494
C5 523
D5 587
E5 659
F5 698
G5 784
A6 880
B6 988
C6 1047
D6 1175
E6 1319
F6 1397
G6 1568
A7 1760
B7 1976
C7 2093
D7 2349
E7 2637
F7 2794
G7 3136
A8 3520
B8 3951
C8 4186
D8 4699
E8 5274
F8 5588
G8 6272
*/

val NOTES_NAMES = listOf(
        "C0",
        "D0",
        "E0",
        "F0",
        "G0",
        "A1",
        "B1",
        "C1",
        "D1",
        "E1",
        "F1",
        "G1",
        "A2",
        "B2",
        "C2",
        "D2",
        "E2",
        "F2",
        "G2",
        "A3",
        "B3",
        "C3",
        "D3",
        "E3",
        "F3",
        "G3",
        "A4",
        "B4",
        "C4",
        "D4",
        "E4",
        "F4",
        "G4",
        "A5",
        "B5",
        "C5",
        "D5",
        "E5",
        "F5",
        "G5",
        "A6",
        "B6",
        "C6",
        "D6",
        "E6",
        "F6",
        "G6",
        "A7",
        "B7",
        "C7",
        "D7",
        "E7",
        "F7",
        "G7",
        "A8",
        "B8",
        "C8",
        "D8",
        "E8",
        "F8",
        "G8"
)

val NOTES_FREQUENCIES = listOf(
        16,
        18,
        21,
        22,
        25,
        28,
        31,
        33,
        37,
        41,
        44,
        49,
        55,
        62,
        65,
        73,
        82,
        87,
        98,
        110,
        123,
        131,
        147,
        165,
        175,
        196,
        220,
        247,
        262,
        294,
        330,
        349,
        392,
        440,
        494,
        523,
        587,
        659,
        698,
        784,
        880,
        988,
        1047,
        1175,
        1319,
        1397,
        1568,
        1760,
        1976,
        2093,
        2349,
        2637,
        2794,
        3136,
        3520,
        3951,
        4186,
        4699,
        5274,
        5588,
        6272
)

val GUARDS_NOTES_NAMES_FREQUENCIES_COUNT = Unit.apply {
    if (NOTES_NAMES.size != NOTES_FREQUENCIES.size) {
        error("Notes names and sizes lists length mismatch!")
    }
}

data class NoteInfo(val name: String, val frequency: Int)

val NOTES_INFOS = NOTES_NAMES.mapIndexed { index, name ->
    NoteInfo(name, NOTES_FREQUENCIES[index])
}.toList()

val NOTES_MAP = mutableMapOf<String, NoteInfo>().apply {
    NOTES_INFOS.forEach { noteInfo ->
        put(noteInfo.name, noteInfo)
    }
}

fun noteFind(name: String): NoteInfo {
    return NOTES_MAP[name] ?: error("Note '$name' not found.")
}

// ----------------

// -------- NOTES TO DIVISIONS --------

const val F_CPU = 9_600_000.0

const val TIMER_FREQUENCY = F_CPU / 1024.0

const val MIN_DIVISIONS = 1

const val MAX_DIVISIONS = 255

const val NOTE_MIN_FREQUENCY_DIVISIONS = MAX_DIVISIONS

const val NOTE_MIN_FREQUENCY = TIMER_FREQUENCY / MAX_DIVISIONS

const val NOTE_MAX_FREQUENCY_DIVISIONS = MIN_DIVISIONS

const val NOTE_MAX_FREQUENCY = TIMER_FREQUENCY / (MIN_DIVISIONS + 1.0)

fun divisionsForNote(frequency: Int): Int {
    if (frequency < NOTE_MIN_FREQUENCY) {
        error("Too low")
        // return NOTE_MIN_FREQUENCY_DIVISIONS.toInt()
    }
    if (frequency > NOTE_MAX_FREQUENCY) {
        error("Too high")
        // return NOTE_MAX_FREQUENCY_DIVISIONS.toInt()
    }
    val r = NOTE_MIN_FREQUENCY_DIVISIONS -
            ((frequency - NOTE_MIN_FREQUENCY) / (NOTE_MAX_FREQUENCY - NOTE_MIN_FREQUENCY)) *
            (MAX_DIVISIONS - MIN_DIVISIONS)
    if (r < NOTE_MAX_FREQUENCY_DIVISIONS || r > NOTE_MIN_FREQUENCY_DIVISIONS) {
        error("Divisions out of range")
    }
    return (r + 0.5).toInt()
}

// ----------------

fun printNote(noteInfo: NoteInfo, fullInfo: Boolean = false) {
    if (noteInfo.frequency < NOTE_MIN_FREQUENCY) {
        println("Note ${noteInfo.name} frequency is too low to get divisions")
        return
    }
    if (noteInfo.frequency > NOTE_MAX_FREQUENCY) {
        println("Note ${noteInfo.name} frequency is too high to get divisions")
        return
    }
    val divisions = divisionsForNote(noteInfo.frequency)
    if (fullInfo) {
        println("${noteInfo.name}: ${noteInfo.frequency}Hz\t\t$divisions -- ${divisions.toDouble() / 4.0}")
    } else {
        println("$divisions, //${noteInfo.name}")
    }
}

fun printNote(name: String, fullInfo: Boolean = false) {
    val noteInfo = noteFind(name)
    printNote(noteInfo, fullInfo)
}

fun printNotes(setName: String, names: List<String>) {
    val infos = names.map { noteFind(it) }
    println(setName)
    for (item in infos) {
        printNote(item, fullInfo  = true)
    }
    for (item in infos) {
        printNote(item, fullInfo  = false)
    }
}

fun main() {
    GUARDS_NOTES_NAMES_FREQUENCIES_COUNT

    val printNotes3 = listOf(
            "D1",
            "A4",
            "B5",
            "C6",
            "F6",
            "G6",
            "A7",
            "B7",
            "C7",
            "D7",
            "E7",
            "F7",
            "G7",
            "A8",
            "B8",
            "C8"
    )

    println("_NMINF: $NOTE_MIN_FREQUENCY")
    println("_NMAXF: $NOTE_MAX_FREQUENCY")
    println()

    printNotes("SET3", printNotes3)
    println()

    printNotes("SET_ALL", NOTES_NAMES)
    println()

    println("end")
}
