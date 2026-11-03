// The cases of src/music/music.test.ts, against the C++ port.
#include "../Source/music/Music.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace keshet;

static int failures = 0;
#define CHECK(cond, ...)                                    \
    do {                                                    \
        if (! (cond)) {                                     \
            ++failures;                                     \
            std::printf ("FAIL %s:%d ", __FILE__, __LINE__); \
            std::printf (__VA_ARGS__);                      \
            std::printf ("\n");                             \
        }                                                   \
    } while (0)

static int scaleIndex (const char* id)
{
    for (int i = 0; i < NUM_SCALES; ++i)
        if (std::strcmp (scales()[(size_t) i].id, id) == 0) return i;
    std::printf ("no scale %s\n", id);
    std::exit (1);
}

static std::string enName (const Spelled& s) { return std::string (LETTERS[s.letter]) + accSymbol (s.acc); }

static std::string spell (int root, const char* id, bool flats)
{
    const Scale& scale = scales()[(size_t) scaleIndex (id)];
    std::string out;
    for (int i = 0; i < scale.numDegrees; ++i)
        out += (i ? " " : "") + enName (spellDegree (root, flats, scale.degrees[(size_t) i]));
    return out;
}

int main()
{
    enum { C = 0, Db = 1, D = 2, Eb = 3, E = 4, Gb = 6, G = 7, Ab = 8, A = 9, Bb = 10, B = 11 };
    struct Case { int root; const char* id; bool flats; const char* expected; };
    const Case cases[] = {
        { C, "ionian", true, "C D E F G A B" },
        { E, "hijaz", true, "E F G♯ A B C D" },
        { Eb, "harmMinor", true, "E♭ F G♭ A♭ B♭ C♭ D" },
        { D, "dorian", true, "D E F G A B C" },
        { Gb, "ionian", true, "F♯ G♯ A♯ B C♯ D♯ E♯" },
        { Gb, "ionian", false, "G♭ A♭ B♭ C♭ D♭ E♭ F" },
        { Db, "lydian", false, "C♯ D♯ E♯ F𝄪 G♯ A♯ B♯" },
        { Db, "phrygian", true, "D♭ E𝄫 F♭ G♭ A♭ B𝄫 C♭" },
        { B, "locrian", true, "B C D E F G A" },
        { A, "hungMinor", true, "A B C D♯ E F G♯" },
        { A, "minPenta", true, "A C D E G" },
        { C, "majPenta", true, "C D E G A" },
        { E, "insen", true, "E F A B D" },
        { D, "hirajoshi", true, "D E F A B♭" },
        { C, "wholeTone", true, "C D E F♯ G♯ B♭" },
        { A, "blues", true, "A C D E♭ E G" },
        { C, "dimHW", true, "C D♭ D♯ E F♯ G A B♭" },
        { C, "dimWH", true, "C D E♭ F G♭ A♭ A B" },
        { G, "bebop", true, "G A B C D E F F♯" },
    };
    for (const auto& c : cases)
    {
        const auto got = spell (c.root, c.id, c.flats);
        CHECK (got == c.expected, "%s root %d: got '%s' want '%s'", c.id, c.root, got.c_str(), c.expected);
    }

    {
        std::string flats, sharps;
        for (int pc : { Db, Eb, Gb, Ab, Bb })
        {
            flats += enName (spellRoot (pc, true)) + " ";
            sharps += enName (spellRoot (pc, false)) + " ";
        }
        CHECK (flats == "D♭ E♭ F♯ A♭ B♭ ", "flat roots: %s", flats.c_str());
        CHECK (sharps == "C♯ D♯ G♭ G♯ A♯ ", "sharp roots: %s", sharps.c_str());
    }

    CHECK (std::strcmp (scales()[(size_t) scaleIndex ("hijaz")].degrees[2].label, "3") == 0, "label");
    CHECK (std::strcmp (scales()[(size_t) scaleIndex ("dorian")].degrees[2].label, "♭3") == 0, "flat label");

    // Steps add up to an octave and never go backwards.
    for (const auto& s : scales())
    {
        CHECK (s.numDegrees >= 5 && s.numDegrees <= 8, "%s has %d degrees", s.id, s.numDegrees);
        for (int i = 1; i < s.numDegrees; ++i)
            CHECK (s.degrees[(size_t) i].semi > s.degrees[(size_t) i - 1].semi, "%s steps", s.id);
        CHECK (s.degrees[(size_t) s.numDegrees - 1].semi < 12, "%s top", s.id);
    }

    {
        int walk[MAX_STEPS];
        int n = walkIndexes (7, Direction::up, Pattern::scale, walk, MAX_STEPS);
        CHECK (n == 8 && walk[0] == 0 && walk[7] == 7, "up walk");
        n = walkIndexes (7, Direction::down, Pattern::scale, walk, MAX_STEPS);
        CHECK (n == 8 && walk[0] == 7 && walk[7] == 0, "down walk");
        CHECK (walkIndexes (7, Direction::updown, Pattern::scale, walk, MAX_STEPS) == 16, "updown walk");
        n = walkIndexes (7, Direction::up, Pattern::arp, walk, MAX_STEPS);
        const int triads[] = { 0, 2, 4, 1, 3, 5 };
        CHECK (std::memcmp (walk, triads, sizeof triads) == 0 && walk[n - 1] == 7, "arp walk");
    }

    {
        Sequence seq;
        buildSequence ({ scaleIndex ("dorian"), D, true, Direction::up, Pattern::scale, 1, Singer::female }, seq);
        std::string names;
        for (int i = 0; i < seq.length; ++i) names += (i ? " " : "") + enName (seq.notes[(size_t) i].spelled);
        CHECK (names == "D E F G A B C D", "D dorian: %s", names.c_str());
        CHECK (seq.notes[7].midi - seq.notes[0].midi == 12, "octave");

        buildSequence ({ scaleIndex ("majPenta"), C, true, Direction::up, Pattern::scale, 1, Singer::female }, seq);
        CHECK (seq.length == 8 && seq.notes[6].rest && seq.notes[7].rest && ! seq.notes[5].rest, "rest padding");
    }

    // The full matrix.
    int combos = 0;
    const Singer singers[] = { Singer::female, Singer::male, Singer::piano, Singer::silent };
    const Direction directions[] = { Direction::up, Direction::down, Direction::updown };
    const Pattern patterns[] = { Pattern::scale, Pattern::thirds, Pattern::arp };
    for (int si = 0; si < NUM_SCALES; ++si)
        for (int rootPc = 0; rootPc < 12; ++rootPc)
            for (bool flats : { true, false })
                for (auto direction : directions)
                    for (auto pattern : patterns)
                        for (int sub = 1; sub <= 4; ++sub)
                            for (auto singer : singers)
                            {
                                Sequence seq;
                                SequenceOptions options { si, rootPc, flats, direction, pattern, sub, singer };
                                buildSequence (options, seq);
                                int walk[MAX_STEPS * 2];
                                const int count = walkIndexes (scales()[(size_t) si].numDegrees, direction, pattern, walk, MAX_STEPS * 2);
                                CHECK (count <= seq.length, "truncated %d > %d", count, seq.length);
                                CHECK (seq.length % (4 * sub) == 0, "bar padding");
                                int low, high;
                                voiceRange (singer, low, high);
                                for (int i = 0; i < seq.length; ++i)
                                {
                                    const auto& note = seq.notes[(size_t) i];
                                    if (note.rest) continue;
                                    CHECK (std::abs (note.spelled.acc) <= 2, "triple accidental");
                                    CHECK (note.midi % 12 == note.spelled.pc, "pitch mismatch");
                                    CHECK (note.midi >= low - 3 && note.midi <= high + 3, "range %d", note.midi);
                                    if (pattern == Pattern::scale)
                                        CHECK (note.midi >= low && note.midi <= high, "plain scale range %d", note.midi);
                                    TimedSyllable parts[5];
                                    const int n = noteSyllables (note.spelled, parts);
                                    CHECK (n >= 1 && n <= 5, "syllables");
                                }
                                ++combos;
                            }
    CHECK (combos == 27 * 12 * 2 * 3 * 3 * 4 * 4, "combos %d", combos);

    {
        auto names = [] (int letter, int acc)
        {
            static const char* const SYL[] = { "do", "re", "mi", "fa", "sol", "la", "si", "di", "ez", "be", "mol" };
            TimedSyllable parts[5];
            const int n = noteSyllables ({ letter, acc, 0 }, parts);
            std::string out;
            for (int i = 0; i < n; ++i) out += (i ? " " : "") + std::string (SYL[(int) parts[i].syllable]);
            return out;
        };
        CHECK (names (0, 0) == "do", "do");
        CHECK (names (0, 1) == "do di ez", "do diez");
        CHECK (names (1, -1) == "re be mol", "re bemol");
        CHECK (names (1, -2) == "re be mol be mol", "double flat");
        CHECK (names (3, 2) == "fa di ez di ez", "double sharp");
        TimedSyllable parts[5];
        const int n = noteSyllables ({ 0, 1, 1 }, parts);
        CHECK (std::abs (parts[0].length - 0.4) < 1e-9 && std::abs (parts[n - 1].start + parts[n - 1].length - 1.0) < 1e-9, "fills the note");
    }

    uint8_t r, g, b;
    oklchToRgb (0.76, 0.15, noteHue (0), r, g, b);
    CHECK (r > g && r > b, "C is red: %d %d %d", r, g, b);

    if (failures)
    {
        std::printf ("%d failures\n", failures);
        return 1;
    }
    std::printf ("music: all passed (%d combos)\n", combos);
    return 0;
}
