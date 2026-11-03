#include "Music.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string_view>

namespace keshet
{
const char* const LETTERS[7] = { "C", "D", "E", "F", "G", "A", "B" };
const char* const HEBREW[7] = { "דו", "רה", "מי", "פה", "סול", "לה", "סי" };
/** Pitch class of each natural letter. Also the major scale. */
const int LETTER_PC[7] = { 0, 2, 4, 5, 7, 9, 11 };
const char* const CATEGORY_HE[NUM_CATEGORIES] = { "מודוסים", "מז'ור ומינור", "מזרח ועולם", "פנטטוני ובלוז", "סימטריים וג'אז" };

double midiToFreq (double midi) { return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0); }

const char* accSymbol (int acc)
{
    switch (acc)
    {
        case -2: return "𝄫";
        case -1: return "♭";
        case 1:  return "♯";
        case 2:  return "𝄪";
        default: return "";
    }
}

namespace
{
using C = Category;

/** To add a scale, add one line (and bump NUM_SCALES). Same order as src/music/scales.data.ts. */
struct Row { const char* id; C cat; const char* he; const char* en; const char* formula; const char* desc; };
const Row SCALE_DATA[NUM_SCALES] = {
    { "ionian", C::modes, "יוני (מז'ור)", "Ionian / Major", "1 2 3 4 5 6 7", "הסולם הבסיסי של המוזיקה המערבית. יציב, בהיר ושמח. על הקלידים הלבנים: דו עד דו." },
    { "dorian", C::modes, "דורי", "Dorian", "1 2 b3 4 5 6 b7", "מינור עם נקודת אור: הדרגה השישית הגבוהה נותנת לו צבע חם. נפוץ בפאנק, ג'אז ורוק. על הקלידים הלבנים: רה עד רה." },
    { "phrygian", C::modes, "פריגי", "Phrygian", "1 b2 b3 4 5 b6 b7", "כהה ומתוח בגלל חצי הטון מעל היסוד. צבע ספרדי ומטאל. במוזיקה הערבית נקרא כורד. על הקלידים הלבנים: מי עד מי." },
    { "lydian", C::modes, "לידי", "Lydian", "1 2 3 #4 5 6 7", "מז'ור עם רביעית מוגבהת. חלומי ומרחף, אהוב על מלחיני סרטים. על הקלידים הלבנים: פה עד פה." },
    { "mixolydian", C::modes, "מיקסולידי", "Mixolydian", "1 2 3 4 5 6 b7", "מז'ור עם שביעית נמוכה. רוק, בלוז ופולק. על הקלידים הלבנים: סול עד סול." },
    { "aeolian", C::modes, "איאולי (מינור טבעי)", "Aeolian / Natural minor", "1 2 b3 4 5 b6 b7", "המינור הטבעי. עצוב, רציני ואפי. על הקלידים הלבנים: לה עד לה." },
    { "locrian", C::modes, "לוקרי", "Locrian", "1 b2 b3 4 b5 b6 b7", "המודוס הכי לא יציב: גם החמישית מונמכת, כך שאין לו אקורד יסוד רגוע. על הקלידים הלבנים: סי עד סי." },

    { "harmMinor", C::minmaj, "מינור הרמוני", "Harmonic minor", "1 2 b3 4 5 b6 7", "מינור עם שביעית גבוהה, שיוצרת קפיצה של טון וחצי בין 6 ל-7. צבע מזרחי-קלאסי. במוזיקה הערבית זה מקאם נהוונד." },
    { "melMinor", C::minmaj, "מינור מלודי", "Melodic minor", "1 2 b3 4 5 6 7", "מינור שהחצי העליון שלו מז'ורי. בסיס להרבה הרמוניה בג'אז." },
    { "harmMajor", C::minmaj, "מז'ור הרמוני", "Harmonic major", "1 2 3 4 5 b6 7", "מז'ור עם שישית נמוכה. מתוק-מר, נשמע כמו מז'ור שמסתיר סוד." },

    { "hijaz", C::east, "חיג'אז (פריגי ערבי)", "Phrygian dominant", "1 b2 3 4 5 b6 b7", "הסולם שמזוהה הכי חזק עם צליל מזרחי: חצי טון ואז טון וחצי בתחילתו. מופיע במוזיקה ערבית, יהודית (אהבה רבה), פלמנקו וקלזמר." },
    { "hijazKar", C::east, "חיג'אז כאר (כפול הרמוני)", "Double harmonic", "1 b2 3 4 5 b6 7", "שני מרווחים של טון וחצי. דרמטי מאוד, נשמע כמו פסקול של מדבר. נקרא גם הסולם הביזנטי." },
    { "ukrDorian", C::east, "דורי אוקראיני (משברך)", "Ukrainian Dorian", "1 2 b3 #4 5 6 b7", "מודוס של תפילה וקלזמר. מינור עם רביעית מוגבהת שמוסיפה בכי ותחינה." },
    { "hungMinor", C::east, "מינור הונגרי", "Hungarian minor", "1 2 b3 #4 5 b6 7", "נקרא גם המינור הצועני. שני מרווחים של טון וחצי נותנים לו צבע חריף." },
    { "persian", C::east, "פרסי", "Persian", "1 b2 3 4 b5 b6 7", "מתח רב בכל הסולם: גם החמישית מונמכת. אקזוטי ולא יציב." },
    { "neapMinor", C::east, "נפוליטני מינור", "Neapolitan minor", "1 b2 b3 4 5 b6 7", "מינור הרמוני עם שנייה נמוכה. כהה ואופראי." },
    { "hirajoshi", C::east, "היראג'ושי (יפני)", "Hirajoshi", "1 2 b3 5 b6", "סולם פנטטוני יפני של חמישה צלילים. רך, עצוב ושקוף, נשמע כמו קוטו." },
    { "insen", C::east, "אין-סן (יפני)", "In-sen", "1 b2 4 5 b7", "סולם יפני של חמישה צלילים עם חצי טון בהתחלה. שקט, מדיטטיבי ומעט מסתורי." },

    { "majPenta", C::penta, "פנטטוני מז'ור", "Major pentatonic", "1 2 3 5 6", "חמישה צלילים בלי חצאי טונים, ולכן כמעט אי אפשר לטעות בו. קאנטרי, פופ ומוזיקת עם מכל העולם." },
    { "minPenta", C::penta, "פנטטוני מינור", "Minor pentatonic", "1 b3 4 5 b7", "הסולם הראשון של כל גיטריסט רוק ובלוז." },
    { "blues", C::penta, "בלוז", "Blues", "1 b3 4 b5 5 b7", "פנטטוני מינור עם הצליל הכחול: החמישית המונמכת שמשמשת כמעבר." },

    { "wholeTone", C::jazz, "טון שלם", "Whole tone", "1 2 3 #4 #5 b7", "רק טונים שלמים, בלי שום חצי טון. אין לו מרכז כובד, ולכן נשמע כמו חלום או קסם (דביסי)." },
    { "dimHW", C::jazz, "מוקטן (חצי-טון)", "Diminished half-whole", "1 b2 #2 3 #4 5 6 b7", "שמונה צלילים שמתחלפים בחצי טון וטון. סימטרי, ומשמש לאלתור על אקורד דומיננטי בג'אז." },
    { "dimWH", C::jazz, "מוקטן (טון-חצי)", "Diminished whole-half", "1 2 b3 4 b5 b6 6 7", "בן הזוג של המוקטן חצי-טון: שמונה צלילים שמתחילים בטון שלם. סימטרי ומרחף, ומתאים לאקורד מוקטן." },
    { "lydDom", C::jazz, "לידי דומיננטי", "Lydian dominant", "1 2 3 #4 5 6 b7", "לידי עם שביעית נמוכה. הצליל של פתיחת הסימפסונס ושל הרבה ג'אז מודרני." },
    { "altered", C::jazz, "אלטרד", "Altered", "1 b2 #2 3 #4 #5 b7", "כל מה שאפשר לשנות באקורד דומיננטי שונה. מתח מקסימלי שמבקש פתרון." },
    { "bebop", C::jazz, "ביבופ דומיננטי", "Bebop dominant", "1 2 3 4 5 6 b7 7", "מיקסולידי עם צליל מעבר נוסף, כך ששמונה צלילים נופלים יפה על פעמות." },
};

int parseFormula (const char* formula, std::array<Degree, MAX_DEGREES>& out)
{
    int count = 0;
    const char* p = formula;
    while (*p && count < MAX_DEGREES)
    {
        while (*p == ' ') ++p;
        if (! *p) break;
        Degree d;
        std::string_view token (p, std::strcspn (p, " "));
        size_t labelLen = 0;
        for (char ch : token)
        {
            const char* piece = nullptr;
            if (ch == 'b') { d.acc -= 1; piece = "♭"; }
            else if (ch == '#') { d.acc += 1; piece = "♯"; }
            else { d.n = ch - '0'; d.label[labelLen++] = ch; continue; }
            const size_t len = std::strlen (piece);
            std::memcpy (d.label + labelLen, piece, len);
            labelLen += len;
        }
        d.label[labelLen] = 0;
        d.semi = LETTER_PC[d.n - 1] + d.acc;
        out[(size_t) count++] = d;
        p += token.size();
    }
    return count;
}
} // namespace

const std::array<Scale, NUM_SCALES>& scales()
{
    static const auto table = []
    {
        std::array<Scale, NUM_SCALES> result {};
        for (int i = 0; i < NUM_SCALES; ++i)
        {
            const Row& row = SCALE_DATA[i];
            Scale& s = result[(size_t) i];
            s.id = row.id;
            s.cat = row.cat;
            s.he = row.he;
            s.en = row.en;
            s.formula = row.formula;
            s.desc = row.desc;
            s.numDegrees = parseFormula (row.formula, s.degrees);
        }
        return result;
    }();
    return table;
}

static int naturalLetterOf (int pc)
{
    for (int i = 0; i < 7; ++i)
        if (LETTER_PC[i] == pc) return i;
    return -1;
}

Spelled spellRoot (int pc, bool preferFlats)
{
    const int natural = naturalLetterOf (pc);
    if (natural >= 0) return { natural, 0, pc };
    const bool useFlat = pc == 6 ? ! preferFlats : preferFlats;
    const int letterPc = mod (pc + (useFlat ? 1 : -1), 12);
    return { naturalLetterOf (letterPc), useFlat ? -1 : 1, pc };
}

Spelled spellDegree (int rootPc, bool preferFlats, const Degree& degree)
{
    const int letter = (spellRoot (rootPc, preferFlats).letter + degree.n - 1) % 7;
    const int pc = mod (rootPc + degree.semi, 12);
    return { letter, mod (pc - LETTER_PC[letter] + 6, 12) - 6, pc };
}

void voiceRange (Singer singer, int& low, int& high)
{
    if (singer == Singer::male) { low = 45; high = 69; } // A2–A4
    else { low = 57; high = 81; }                         // A3–A5
}

int walkIndexes (int n, Direction direction, Pattern pattern, int* out, int capacity)
{
    int up[MAX_STEPS];
    int count = 0;
    const int span = pattern == Pattern::thirds ? 2 : pattern == Pattern::arp ? 3 : 1;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < span; ++j)
            up[count++] = i + 2 * j;
    up[count++] = n;

    int total = 0;
    auto push = [&] (int v) { if (total < capacity) out[total++] = v; };
    if (direction != Direction::down)
        for (int i = 0; i < count; ++i) push (up[i]);
    if (direction != Direction::up)
        for (int i = count - 1; i >= 0; --i) push (up[i]);
    return total;
}

int fitRootMidi (int rootPc, Singer singer, int lowSemi, int highSemi)
{
    int low, high;
    voiceRange (singer, low, high);
    int best = -1;
    int bestOverflow = std::numeric_limits<int>::max();
    for (int midi = rootPc + 24; midi <= high; midi += 12)
    {
        const int overflow = std::max (0, low - (midi + lowSemi)) + std::max (0, midi + highSemi - high);
        if (overflow < bestOverflow)
        {
            best = midi;
            bestOverflow = overflow;
        }
    }
    return best;
}

void buildSequence (const SequenceOptions& options, Sequence& out)
{
    const Scale& scale = scales()[(size_t) std::clamp (options.scaleIndex, 0, NUM_SCALES - 1)];
    const int n = scale.numDegrees;
    const int sub = std::clamp (options.sub, 1, 4);
    int indexes[MAX_STEPS];
    const int count = walkIndexes (n, options.direction, options.pattern, indexes, MAX_STEPS);

    int semis[MAX_STEPS];
    int lowSemi = std::numeric_limits<int>::max(), highSemi = std::numeric_limits<int>::min();
    for (int i = 0; i < count; ++i)
    {
        const int index = indexes[i];
        semis[i] = scale.degrees[(size_t) mod (index, n)].semi + 12 * (index / n);
        lowSemi = std::min (lowSemi, semis[i]);
        highSemi = std::max (highSemi, semis[i]);
    }
    const int rootMidi = fitRootMidi (options.rootPc, options.singer, lowSemi, highSemi);

    const int bar = BEATS_PER_BAR * sub;
    const int padded = std::min (MAX_STEPS, (count + bar - 1) / bar * bar);
    out.length = padded;
    for (int i = 0; i < padded; ++i)
    {
        SeqNote& note = out.notes[(size_t) i];
        if (i >= count) { note = {}; continue; }
        const int degreeIndex = mod (indexes[i], n);
        note.rest = false;
        note.midi = rootMidi + semis[i];
        note.degreeIndex = degreeIndex;
        note.spelled = spellDegree (options.rootPc, options.preferFlats, scale.degrees[(size_t) degreeIndex]);
    }
}

int noteSyllables (const Spelled& spelled, TimedSyllable* out)
{
    static const Syllable LETTER_SYLLABLE[7] = { Syllable::do_, Syllable::re, Syllable::mi, Syllable::fa, Syllable::sol, Syllable::la, Syllable::si };
    constexpr double BASE_SHARE = 0.4;
    const Syllable base = LETTER_SYLLABLE[spelled.letter];
    if (spelled.acc == 0)
    {
        out[0] = { base, 0.0, 1.0 };
        return 1;
    }
    const Syllable first = spelled.acc > 0 ? Syllable::di : Syllable::be;
    const Syllable second = spelled.acc > 0 ? Syllable::ez : Syllable::mol;
    const int parts = 2 * std::abs (spelled.acc);
    const double share = (1.0 - BASE_SHARE) / parts;
    out[0] = { base, 0.0, BASE_SHARE };
    for (int i = 0; i < parts; ++i)
        out[i + 1] = { i % 2 == 0 ? first : second, BASE_SHARE + share * i, share };
    return parts + 1;
}

void oklchToRgb (double l, double c, double hueDeg, uint8_t& r, uint8_t& g, uint8_t& b)
{
    const double h = hueDeg * 3.14159265358979323846 / 180.0;
    const double A = c * std::cos (h), B = c * std::sin (h);
    const double l_ = l + 0.3963377774 * A + 0.2158037573 * B;
    const double m_ = l - 0.1055613458 * A - 0.0638541728 * B;
    const double s_ = l - 0.0894841775 * A - 1.2914855480 * B;
    const double L = l_ * l_ * l_, M = m_ * m_ * m_, S = s_ * s_ * s_;
    const double lin[3] = {
        +4.0767416621 * L - 3.3077115913 * M + 0.2309699292 * S,
        -1.2684380046 * L + 2.6097574011 * M - 0.3413193965 * S,
        -0.0041960863 * L - 0.7034186147 * M + 1.7076147010 * S,
    };
    uint8_t* outs[3] = { &r, &g, &b };
    for (int i = 0; i < 3; ++i)
    {
        const double x = std::clamp (lin[i], 0.0, 1.0);
        const double v = x <= 0.0031308 ? 12.92 * x : 1.055 * std::pow (x, 1.0 / 2.4) - 0.055;
        *outs[i] = (uint8_t) std::lround (std::clamp (v, 0.0, 1.0) * 255.0);
    }
}
} // namespace keshet
