#pragma once

// The music logic of src/music, ported 1:1. No JUCE and no heap use after startup,
// so the audio thread can rebuild a sequence inside processBlock.

#include <array>
#include <cstdint>

namespace keshet
{
constexpr int mod (int n, int m) { return ((n % m) + m) % m; }
double midiToFreq (double midi);

extern const char* const LETTERS[7];
extern const char* const HEBREW[7];
extern const int LETTER_PC[7];

/** A note as written: a letter plus an accidental. */
struct Spelled
{
    int letter = 0; // index into LETTERS
    int acc = 0;    // -2..2
    int pc = 0;     // 0..11
};

const char* accSymbol (int acc);

struct Degree
{
    int n = 1;    // 1..7
    int acc = 0;
    int semi = 0; // semitones above the root
    char label[8] {}; // UTF-8, "♭3"
};

constexpr int MAX_DEGREES = 8;

enum class Category { modes, minmaj, east, penta, jazz };
constexpr int NUM_CATEGORIES = 5;
extern const char* const CATEGORY_HE[NUM_CATEGORIES];

struct Scale
{
    const char* id;
    Category cat;
    const char* he;
    const char* en;
    const char* formula;
    const char* desc;
    int numDegrees = 0;
    std::array<Degree, MAX_DEGREES> degrees {};
};

constexpr int NUM_SCALES = 27;
const std::array<Scale, NUM_SCALES>& scales();

/** Black-key roots default to flats, except F♯. `preferFlats` false swaps all of them. */
Spelled spellRoot (int pc, bool preferFlats);
/** Degree n takes the letter root+n−1; the accidental is whatever closes the gap. */
Spelled spellDegree (int rootPc, bool preferFlats, const Degree& degree);

enum class Direction { up, down, updown };
enum class Pattern { scale, thirds, arp };
enum class Singer { female, male, piano, silent };

constexpr int BEATS_PER_BAR = 4;

struct SeqNote
{
    bool rest = true; // pads the last bar of 4/4
    int midi = 0;
    int degreeIndex = 0;
    Spelled spelled;
};

// Longest: arpeggio up and down an 8-note scale is 50 notes, padded to a 16-step bar.
constexpr int MAX_STEPS = 64;

struct Sequence
{
    int length = 0;
    std::array<SeqNote, MAX_STEPS> notes {};
};

struct SequenceOptions
{
    int scaleIndex = 0;
    int rootPc = 0;
    bool preferFlats = true;
    Direction direction = Direction::up;
    Pattern pattern = Pattern::scale;
    int sub = 1; // 1..4 notes per beat
    Singer singer = Singer::female;

    bool operator== (const SequenceOptions&) const = default;
};

void voiceRange (Singer singer, int& low, int& high);
/** The walk as scale-step indexes. Index n is the root an octave up. Returns the count. */
int walkIndexes (int n, Direction direction, Pattern pattern, int* out, int capacity);
/** Picks the octave of the root that overflows the singer's range the least, lowest first. */
int fitRootMidi (int rootPc, Singer singer, int lowSemi, int highSemi);
/** The notes of one pass, padded with rests to whole bars. */
void buildSequence (const SequenceOptions& options, Sequence& out);

enum class Syllable { do_, re, mi, fa, sol, la, si, di, ez, be, mol };

struct TimedSyllable
{
    Syllable syllable;
    double start;  // fraction of the note
    double length; // fraction of the note
};

/** The full sung name, split across the note: "רה במול" is re + be + mol. Returns the count (max 5). */
int noteSyllables (const Spelled& spelled, TimedSyllable* out);

/** Chromatic wheel: C is red, then 30° for each semitone. */
inline double noteHue (int pc) { return (25 + pc * 30) % 360; }
/** oklch to 8-bit sRGB, clipped to gamut. */
void oklchToRgb (double l, double c, double hueDeg, uint8_t& r, uint8_t& g, uint8_t& b);
} // namespace keshet
