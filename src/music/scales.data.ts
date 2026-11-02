export type CategoryId = 'modes' | 'minmaj' | 'east' | 'penta' | 'jazz';

export interface ScaleData {
  id: string;
  cat: CategoryId;
  he: string;
  en: string;
  formula: string;
  desc: string;
}

export const CATEGORIES: { id: CategoryId; he: string }[] = [
  { id: 'modes', he: 'מודוסים' },
  { id: 'minmaj', he: "מז'ור ומינור" },
  { id: 'east', he: 'מזרח ועולם' },
  { id: 'penta', he: 'פנטטוני ובלוז' },
  { id: 'jazz', he: "סימטריים וג'אז" },
];

/** To add a scale, add one line. */
export const SCALE_DATA: ScaleData[] = [
  { id: 'ionian', cat: 'modes', he: "יוני (מז'ור)", en: 'Ionian / Major', formula: '1 2 3 4 5 6 7', desc: 'הסולם הבסיסי של המוזיקה המערבית. יציב, בהיר ושמח. על הקלידים הלבנים: דו עד דו.' },
  { id: 'dorian', cat: 'modes', he: 'דורי', en: 'Dorian', formula: '1 2 b3 4 5 6 b7', desc: "מינור עם נקודת אור: הדרגה השישית הגבוהה נותנת לו צבע חם. נפוץ בפאנק, ג'אז ורוק. על הקלידים הלבנים: רה עד רה." },
  { id: 'phrygian', cat: 'modes', he: 'פריגי', en: 'Phrygian', formula: '1 b2 b3 4 5 b6 b7', desc: 'כהה ומתוח בגלל חצי הטון מעל היסוד. צבע ספרדי ומטאל. במוזיקה הערבית נקרא כורד. על הקלידים הלבנים: מי עד מי.' },
  { id: 'lydian', cat: 'modes', he: 'לידי', en: 'Lydian', formula: '1 2 3 #4 5 6 7', desc: "מז'ור עם רביעית מוגבהת. חלומי ומרחף, אהוב על מלחיני סרטים. על הקלידים הלבנים: פה עד פה." },
  { id: 'mixolydian', cat: 'modes', he: 'מיקסולידי', en: 'Mixolydian', formula: '1 2 3 4 5 6 b7', desc: "מז'ור עם שביעית נמוכה. רוק, בלוז ופולק. על הקלידים הלבנים: סול עד סול." },
  { id: 'aeolian', cat: 'modes', he: 'איאולי (מינור טבעי)', en: 'Aeolian / Natural minor', formula: '1 2 b3 4 5 b6 b7', desc: 'המינור הטבעי. עצוב, רציני ואפי. על הקלידים הלבנים: לה עד לה.' },
  { id: 'locrian', cat: 'modes', he: 'לוקרי', en: 'Locrian', formula: '1 b2 b3 4 b5 b6 b7', desc: 'המודוס הכי לא יציב: גם החמישית מונמכת, כך שאין לו אקורד יסוד רגוע. על הקלידים הלבנים: סי עד סי.' },

  { id: 'harmMinor', cat: 'minmaj', he: 'מינור הרמוני', en: 'Harmonic minor', formula: '1 2 b3 4 5 b6 7', desc: 'מינור עם שביעית גבוהה, שיוצרת קפיצה של טון וחצי בין 6 ל-7. צבע מזרחי-קלאסי. במוזיקה הערבית זה מקאם נהוונד.' },
  { id: 'melMinor', cat: 'minmaj', he: 'מינור מלודי', en: 'Melodic minor', formula: '1 2 b3 4 5 6 7', desc: "מינור שהחצי העליון שלו מז'ורי. בסיס להרבה הרמוניה בג'אז." },
  { id: 'harmMajor', cat: 'minmaj', he: "מז'ור הרמוני", en: 'Harmonic major', formula: '1 2 3 4 5 b6 7', desc: "מז'ור עם שישית נמוכה. מתוק-מר, נשמע כמו מז'ור שמסתיר סוד." },

  { id: 'hijaz', cat: 'east', he: "חיג'אז (פריגי ערבי)", en: 'Phrygian dominant', formula: '1 b2 3 4 5 b6 b7', desc: 'הסולם שמזוהה הכי חזק עם צליל מזרחי: חצי טון ואז טון וחצי בתחילתו. מופיע במוזיקה ערבית, יהודית (אהבה רבה), פלמנקו וקלזמר.' },
  { id: 'hijazKar', cat: 'east', he: "חיג'אז כאר (כפול הרמוני)", en: 'Double harmonic', formula: '1 b2 3 4 5 b6 7', desc: 'שני מרווחים של טון וחצי. דרמטי מאוד, נשמע כמו פסקול של מדבר. נקרא גם הסולם הביזנטי.' },
  { id: 'ukrDorian', cat: 'east', he: 'דורי אוקראיני (משברך)', en: 'Ukrainian Dorian', formula: '1 2 b3 #4 5 6 b7', desc: 'מודוס של תפילה וקלזמר. מינור עם רביעית מוגבהת שמוסיפה בכי ותחינה.' },
  { id: 'hungMinor', cat: 'east', he: 'מינור הונגרי', en: 'Hungarian minor', formula: '1 2 b3 #4 5 b6 7', desc: 'נקרא גם המינור הצועני. שני מרווחים של טון וחצי נותנים לו צבע חריף.' },
  { id: 'persian', cat: 'east', he: 'פרסי', en: 'Persian', formula: '1 b2 3 4 b5 b6 7', desc: 'מתח רב בכל הסולם: גם החמישית מונמכת. אקזוטי ולא יציב.' },
  { id: 'neapMinor', cat: 'east', he: 'נפוליטני מינור', en: 'Neapolitan minor', formula: '1 b2 b3 4 5 b6 7', desc: 'מינור הרמוני עם שנייה נמוכה. כהה ואופראי.' },
  { id: 'hirajoshi', cat: 'east', he: "היראג'ושי (יפני)", en: 'Hirajoshi', formula: '1 2 b3 5 b6', desc: 'סולם פנטטוני יפני של חמישה צלילים. רך, עצוב ושקוף, נשמע כמו קוטו.' },
  { id: 'insen', cat: 'east', he: 'אין-סן (יפני)', en: 'In-sen', formula: '1 b2 4 5 b7', desc: 'סולם יפני של חמישה צלילים עם חצי טון בהתחלה. שקט, מדיטטיבי ומעט מסתורי.' },

  { id: 'majPenta', cat: 'penta', he: "פנטטוני מז'ור", en: 'Major pentatonic', formula: '1 2 3 5 6', desc: 'חמישה צלילים בלי חצאי טונים, ולכן כמעט אי אפשר לטעות בו. קאנטרי, פופ ומוזיקת עם מכל העולם.' },
  { id: 'minPenta', cat: 'penta', he: 'פנטטוני מינור', en: 'Minor pentatonic', formula: '1 b3 4 5 b7', desc: 'הסולם הראשון של כל גיטריסט רוק ובלוז.' },
  { id: 'blues', cat: 'penta', he: 'בלוז', en: 'Blues', formula: '1 b3 4 b5 5 b7', desc: 'פנטטוני מינור עם הצליל הכחול: החמישית המונמכת שמשמשת כמעבר.' },

  { id: 'wholeTone', cat: 'jazz', he: 'טון שלם', en: 'Whole tone', formula: '1 2 3 #4 #5 b7', desc: 'רק טונים שלמים, בלי שום חצי טון. אין לו מרכז כובד, ולכן נשמע כמו חלום או קסם (דביסי).' },
  { id: 'dimHW', cat: 'jazz', he: 'מוקטן (חצי-טון)', en: 'Diminished half-whole', formula: '1 b2 #2 3 #4 5 6 b7', desc: "שמונה צלילים שמתחלפים בחצי טון וטון. סימטרי, ומשמש לאלתור על אקורד דומיננטי בג'אז." },
  { id: 'dimWH', cat: 'jazz', he: 'מוקטן (טון-חצי)', en: 'Diminished whole-half', formula: '1 2 b3 4 b5 b6 6 7', desc: 'בן הזוג של המוקטן חצי-טון: שמונה צלילים שמתחילים בטון שלם. סימטרי ומרחף, ומתאים לאקורד מוקטן.' },
  { id: 'lydDom', cat: 'jazz', he: 'לידי דומיננטי', en: 'Lydian dominant', formula: '1 2 3 #4 5 6 b7', desc: "לידי עם שביעית נמוכה. הצליל של פתיחת הסימפסונס ושל הרבה ג'אז מודרני." },
  { id: 'altered', cat: 'jazz', he: 'אלטרד', en: 'Altered', formula: '1 b2 #2 3 #4 #5 b7', desc: 'כל מה שאפשר לשנות באקורד דומיננטי שונה. מתח מקסימלי שמבקש פתרון.' },
  { id: 'bebop', cat: 'jazz', he: 'ביבופ דומיננטי', en: 'Bebop dominant', formula: '1 2 3 4 5 6 b7 7', desc: 'מיקסולידי עם צליל מעבר נוסף, כך ששמונה צלילים נופלים יפה על פעמות.' },
];
