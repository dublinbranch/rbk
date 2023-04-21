#include "codes.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/QStacker/qstacker.h"
#include <QDebug>
#include <QMap>

//This whole table is wrong and leftover of aggressive optimization in gaisensha where each nation was a bit in a 256bit register
//we now just use to check if a 2 letter code is valid Nation
const mapV2<QByteArray, quint16>& getNationISO2() {
	const static mapV2<QByteArray, quint16> nations =
	    {{
			 {"AD", 1},
			 {"AE", 2},
			 {"UAE", 2},
			 {"AF", 3},
			 {"AG", 4},
			 {"AI", 5},
			 {"AL", 6},
			 {"AM", 7},
			 {"AO", 8},
			 {"AQ", 9},
			 {"AR", 10},
			 {"AS", 11},
			 {"AT", 12},
			 {"AU", 13},
			 {"AW", 14},
			 {"AX", 15},
			 {"AZ", 16},
			 {"BA", 17},
			 {"BB", 18},
			 {"BD", 19},
			 {"BE", 20},
			 {"BF", 21},
			 {"BG", 22},
			 {"BH", 23},
			 {"BI", 24},
			 {"BJ", 25},
			 {"BL", 26},
			 {"BM", 27},
			 {"BN", 28},
			 {"BO", 29},
			 {"BQ", 30},
			 {"BR", 31},
			 {"BS", 32},
			 {"BT", 33},
			 {"BV", 34},
			 {"BW", 35},
			 {"BY", 36},
			 {"BZ", 37},
			 {"CA", 38},
			 {"CC", 39},
			 {"CD", 40},
			 {"CF", 41},
			 {"CG", 42},
			 {"CH", 43},
			 {"CI", 44},
			 {"CK", 45},
			 {"CL", 46},
			 {"CM", 47},
			 {"CN", 48},
			 {"CO", 49},
			 {"CR", 50},
			 {"CU", 51},
			 {"CV", 52},
			 {"CW", 53},
			 {"CX", 54},
			 {"CY", 55},
			 {"CZ", 56},
			 {"DE", 57},
			 {"DJ", 58},
			 {"DK", 59},
			 {"DM", 60},
			 {"DO", 61},
			 {"DZ", 62},
			 {"EC", 63},
			 {"EE", 64},
			 {"EG", 65},
			 {"EH", 66},
			 {"ER", 67},
			 {"ES", 68},
			 {"ET", 69},
			 {"FI", 70},
			 {"FJ", 71},
			 {"FK", 72},
			 {"FM", 73},
			 {"FO", 74},
			 {"FR", 75},
			 {"GA", 76},
			 {"GB", 77}, //this is the geographic toponim of the main island
			 {"UK", 77}, //UK this is the actual politic entity (which comprise GB and many more island ecc)
			 {"GD", 78},
			 {"GE", 79},
			 {"GF", 80},
			 {"GG", 81},
			 {"GH", 82},
			 {"GI", 83},
			 {"GL", 84},
			 {"GM", 85},
			 {"GN", 86},
			 {"GP", 87},
			 {"GQ", 88},
			 {"GR", 89},
			 {"GS", 90},
			 {"GT", 91},
			 {"GU", 92},
			 {"GW", 93},
			 {"GY", 94},
			 {"HK", 95},
			 {"HM", 96},
			 {"HN", 97},
			 {"HR", 98},
			 {"HT", 99},
			 {"HU", 100},
			 {"ID", 101},
			 {"IE", 102},
			 {"IL", 103},
			 {"IM", 104},
			 {"IN", 105},
			 {"IO", 106},
			 {"IQ", 107},
			 {"IR", 108},
			 {"IS", 109},
			 {"IT", 110},
			 {"JE", 111},
			 {"JM", 112},
			 {"JO", 113},
			 {"JP", 114},
			 {"KE", 115},
			 {"KG", 116},
			 {"KH", 117},
			 {"KI", 118},
			 {"KM", 119},
			 {"KN", 120},
			 {"KP", 121},
			 {"KR", 122},
			 {"KW", 123},
			 {"KY", 124},
			 {"KZ", 125},
			 {"LA", 126},
			 {"LB", 127},
			 {"LC", 128},
			 {"LI", 129},
			 {"LK", 130},
			 {"LR", 131},
			 {"LS", 132},
			 {"LT", 133},
			 {"LU", 134},
			 {"LV", 135},
			 {"LY", 136},
			 {"MA", 137},
			 {"MC", 138},
			 {"MD", 139},
			 {"ME", 140},
			 {"MF", 141},
			 {"MG", 142},
			 {"MH", 143},
			 {"MK", 144},
			 {"ML", 145},
			 {"MM", 146},
			 {"MN", 147},
			 {"MO", 148},
			 {"MP", 149},
			 {"MQ", 150},
			 {"MR", 151},
			 {"MS", 152},
			 {"MT", 153},
			 {"MU", 154},
			 {"MV", 155},
			 {"MW", 156},
			 {"MX", 157},
			 {"MY", 158},
			 {"MZ", 159},
			 {"NA", 160},
			 {"NC", 161},
			 {"NE", 162},
			 {"NF", 163},
			 {"NG", 164},
			 {"NI", 165},
			 {"NL", 166},
			 {"NO", 167},
			 {"NP", 168},
			 {"NR", 169},
			 {"NU", 170},
			 {"NZ", 171},
			 {"OM", 172},
			 {"PA", 173},
			 {"PE", 174},
			 {"PF", 175},
			 {"PG", 176},
			 {"PH", 177},
			 {"PK", 178},
			 {"PL", 179},
			 {"PM", 180},
			 {"PN", 181},
			 {"PR", 182},
			 {"PS", 183},
			 {"PT", 184},
			 {"PW", 185},
			 {"PY", 186},
			 {"QA", 187},
			 {"RE", 188},
			 {"RO", 189},
			 {"RS", 190},
			 {"RU", 191},
			 {"RW", 192},
			 {"SA", 193},
			 {"SB", 194},
			 {"SC", 195},
			 {"SD", 196},
			 {"SE", 197},
			 {"SG", 198},
			 {"SH", 199},
			 {"SI", 200},
			 {"SJ", 201},
			 {"SK", 202},
			 {"SL", 203},
			 {"SM", 204},
			 {"SN", 205},
			 {"SO", 206},
			 {"SR", 207},
			 {"SS", 208},
			 {"ST", 209},
			 {"SV", 210},
			 {"SX", 211},
			 {"SY", 212},
			 {"SZ", 213},
			 {"TC", 214},
			 {"TD", 215},
			 {"TF", 216},
			 {"TG", 217},
			 {"TH", 218},
			 {"TJ", 219},
			 {"TK", 220},
			 {"TL", 221},
			 {"TM", 222},
			 {"TN", 223},
			 {"TO", 224},
			 {"TR", 225},
			 {"TT", 226},
			 {"TV", 227},
			 {"TW", 228},
			 {"TZ", 229},
			 {"UA", 230},
			 {"UG", 231},
			 {"UM", 232},
			 {"US", 233},
			 {"UY", 234},
			 {"UZ", 235},
			 {"VA", 236},
			 {"VC", 237},
			 {"VE", 238},
			 {"VG", 239},
			 {"VI", 240},
			 {"VN", 241},
			 {"VU", 242},
			 {"WF", 243},
			 {"WS", 244},
			 {"YE", 245},
			 {"YT", 246},
			 {"ZA", 247},
			 {"ZM", 248},
			 {"ZW", 249},
		 },
		 {}};

	return nations;
}

const QMap<QString, QString>& getNationsIsoCodes() {
	static QMap<QString, QString> mappa = {
	    {"canada", "CA"},
	    {"germany", "DE"},
	    {"spain", "ES"},
	    {"france", "FR"},
	    {"united kingdom", "UK"},
	    {"italy", "IT"},
	    {"ireland", "IE"},
	    {"mexico", "MX"},
	    {"new zealand", "NZ"},
	    {"netherlands", "NL"},
	    {"netherlands, the", "NL"},
	    {"norway", "NO"},
	    {"brazil", "BR"},
	    {"hong kong", "HK"},
	    {"denmark", "DK"},
	    {"austria", "AT"},
	    {"australia", "AU"},
	    {"united states", "US"},
	    {"andorra", "AD"},
	    {"united arab emirates", "AE"},
	    {"afghanistan", "AF"},
	    {"antigua and barbuda", "AG"},
	    {"anguilla", "AI"},
	    {"albania", "AL"},
	    {"armenia", "AM"},
	    {"angola", "AO"},
	    {"antarctica", "AQ"},
	    {"argentina", "AR"},
	    {"american samoa", "AS"},
	    {"austria", "AT"},
	    {"australia", "AU"},
	    {"aruba", "AW"},
	    {"åland", "AX"},
	    {"azerbaijan", "AZ"},
	    {"bosnia and herzegovina", "BA"},
	    {"barbados", "BB"},
	    {"bangladesh", "BD"},
	    {"belgium", "BE"},
	    {"burkina faso", "BF"},
	    {"bulgaria", "BG"},
	    {"bahrain", "BH"},
	    {"burundi", "BI"},
	    {"benin", "BJ"},
	    {"saint-barthélemy", "BL"},
	    {"bermuda", "BM"},
	    {"brunei", "BN"},
	    {"bolivia", "BO"},
	    {"bonaire, sint eustatius, and saba", "BQ"},
	    {"brazil", "BR"},
	    {"bahamas", "BS"},
	    {"bhutan", "BT"},
	    {"botswana", "BW"},
	    {"belarus", "BY"},
	    {"belize", "BZ"},
	    {"canada", "CA"},
	    {"cocos [keeling] islands", "CC"},
	    {"congo", "CD"},
	    {"central african republic", "CF"},
	    {"republic of the congo", "CG"},
	    {"switzerland", "CH"},
	    {"ivory coast", "CI"},
	    {"côte d'ivoire", "CI"},
	    {"cook islands", "CK"},
	    {"chile", "CL"},
	    {"cameroon", "CM"},
	    {"china", "CN"},
	    {"colombia", "CO"},
	    {"costa rica", "CR"},
	    {"cuba", "CU"},
	    {"cape verde", "CV"},
	    {"curaçao", "CW"},
	    {"christmas island", "CX"},
	    {"cyprus", "CY"},
	    {"czech republic", "CZ"},
	    {"czechia", "CZ"},
	    {"germany", "DE"},
	    {"djibouti", "DJ"},
	    {"denmark", "DK"},
	    {"dominica", "DM"},
	    {"dominican republic", "DO"},
	    {"algeria", "DZ"},
	    {"ecuador", "EC"},
	    {"estonia", "EE"},
	    {"egypt", "EG"},
	    {"eritrea", "ER"},
	    {"spain", "ES"},
	    {"ethiopia", "ET"},
	    {"finland", "FI"},
	    {"fiji", "FJ"},
	    {"falkland islands", "FK"},
	    {"federated states of micronesia", "FM"},
	    {"faroe islands", "FO"},
	    {"france", "FR"},
	    {"gabon", "GA"},
	    {"grenada", "GD"},
	    {"georgia", "GE"},
	    {"french guiana", "GF"},
	    {"guernsey", "GG"},
	    {"ghana", "GH"},
	    {"gibraltar", "GI"},
	    {"greenland", "GL"},
	    {"gambia", "GM"},
	    {"guinea", "GN"},
	    {"guadeloupe", "GP"},
	    {"equatorial guinea", "GQ"},
	    {"greece", "GR"},
	    {"south georgia and the south sandwich islands", "GS"},
	    {"guatemala", "GT"},
	    {"guam", "GU"},
	    {"guinea-bissau", "GW"},
	    {"guyana", "GY"},
	    {"hong kong", "HK"},
	    {"honduras", "HN"},
	    {"croatia", "HR"},
	    {"haiti", "HT"},
	    {"hong kong sar", "HK"},
	    {"hungary", "HU"},
	    {"indonesia", "ID"},
	    {"ireland", "IE"},
	    {"israel", "IL"},
	    {"isle of man", "IM"},
	    {"india", "IN"},
	    {"british indian ocean territory", "IO"},
	    {"iraq", "IQ"},
	    {"iran", "IR"},
	    {"iceland", "IS"},
	    {"italy", "IT"},
	    {"jersey", "JE"},
	    {"jamaica", "JM"},
	    {"hashemite kingdom of jordan", "JO"},
		{"jordan", "JO"},
	    {"japan", "JP"},
	    {"kenya", "KE"},
	    {"kyrgyzstan", "KG"},
	    {"cambodia", "KH"},
	    {"kiribati", "KI"},
	    {"comoros", "KM"},
	    {"saint kitts and nevis", "KN"},
	    {"north korea", "KP"},
		{"south korea", "KR"},
	    {"republic of korea", "KR"},
	    {"kuwait", "KW"},
	    {"cayman islands", "KY"},
	    {"kazakhstan", "KZ"},
	    {"laos", "LA"},
	    {"lebanon", "LB"},
	    {"saint lucia", "LC"},
	    {"liechtenstein", "LI"},
	    {"sri lanka", "LK"},
	    {"liberia", "LR"},
	    {"lesotho", "LS"},
	    {"republic of lithuania", "LT"},
	    {"lithuania", "LT"},
	    {"luxembourg", "LU"},
	    {"latvia", "LV"},
	    {"libya", "LY"},
	    {"morocco", "MA"},
	    {"monaco", "MC"},
	    {"republic of moldova", "MD"},
	    {"montenegro", "ME"},
	    {"saint martin", "MF"},
	    {"madagascar", "MG"},
	    {"marshall islands", "MH"},
	    {"macedonia", "MK"},
	    {"nord macedonia", "MK"},
	    {"north macedonia", "MK"},
	    {"republic of north macedonia", "MK"},
	    {"mali", "ML"},
	    {"myanmar [burma]", "MM"},
	    {"mongolia", "MN"},
	    {"macao", "MO"},
	    {"northern mariana islands", "MP"},
	    {"martinique", "MQ"},
	    {"mauritania", "MR"},
	    {"montserrat", "MS"},
	    {"malta", "MT"},
	    {"mauritius", "MU"},
	    {"maldives", "MV"},
	    {"malawi", "MW"},
	    {"mexico", "MX"},
	    {"malaysia", "MY"},
	    {"mozambique", "MZ"},
	    {"namibia", "NA"},
	    {"new caledonia", "NC"},
	    {"niger", "NE"},
	    {"norfolk island", "NF"},
	    {"nigeria", "NG"},
	    {"nicaragua", "NI"},
	    {"netherlands", "NL"},
	    {"norway", "NO"},
	    {"nepal", "NP"},
	    {"nauru", "NR"},
	    {"niue", "NU"},
	    {"new zealand", "NZ"},
	    {"oman", "OM"},
	    {"panama", "PA"},
	    {"peru", "PE"},
	    {"french polynesia", "PF"},
	    {"papua new guinea", "PG"},
	    {"philippines", "PH"},
	    {"pakistan", "PK"},
	    {"poland", "PL"},
	    {"saint pierre and miquelon", "PM"},
	    {"pitcairn islands", "PN"},
	    {"puerto rico", "PR"},
	    {"palestine", "PS"},
	    {"portugal", "PT"},
	    {"palau", "PW"},
	    {"paraguay", "PY"},
	    {"qatar", "QA"},
	    {"réunion", "RE"},
	    {"reunion", "RE"},
	    {"romania", "RO"},
	    {"serbia", "RS"},
	    {"russia", "RU"},
	    {"rwanda", "RW"},
	    {"saudi arabia", "SA"},
	    {"solomon islands", "SB"},
	    {"seychelles", "SC"},
	    {"sudan", "SD"},
	    {"sweden", "SE"},
	    {"singapore", "SG"},
	    {"saint helena", "SH"},
	    {"slovenia", "SI"},
	    {"svalbard and jan mayen", "SJ"},
	    {"slovak republic", "SK"},
	    {"slovakia", "SK"},
	    {"sierra leone", "SL"},
	    {"san marino", "SM"},
	    {"senegal", "SN"},
	    {"somalia", "SO"},
	    {"suriname", "SR"},
	    {"south sudan", "SS"},
	    {"são tomé and príncipe", "ST"},
	    {"el salvador", "SV"},
	    {"sint maarten", "SX"},
	    {"syria", "SY"},
	    {"swaziland", "SZ"},
		{"turks & caicos islands", "TC"},
	    {"turks and caicos islands", "TC"},
	    {"chad", "TD"},
	    {"french southern territories", "TF"},
	    {"togo", "TG"},
	    {"thailand", "TH"},
	    {"tajikistan", "TJ"},
	    {"tokelau", "TK"},
	    {"east timor", "TL"},
	    {"turkmenistan", "TM"},
	    {"tunisia", "TN"},
	    {"tonga", "TO"},
	    {"turkey", "TR"},
		{"türkiye", "TR"},
	    {"trinidad and tobago", "TT"},
		{"trinidad & tobago", "TT"},
		{"repubblica di trinidad e tobago", "TT"},
	    {"tuvalu", "TV"},
	    {"taiwan", "TW"},
	    {"tanzania", "TZ"},
	    {"ukraine", "UA"},
	    {"uganda", "UG"},
	    {"u.s. minor outlying islands", "UM"},
	    {"united states", "US"},
	    {"uruguay", "UY"},
	    {"uzbekistan", "UZ"},
	    {"vatican city", "VA"},
	    {"saint vincent and the grenadines", "VC"},
		{"st. vincent & grenadines", "VC"},
	    {"venezuela", "VE"},
	    {"british virgin islands", "VG"},
	    {"u.s. virgin islands", "VI"},
	    {"vietnam", "VN"},
	    {"vanuatu", "VU"},
	    {"wallis and futuna", "WF"},
	    {"samoa", "WS"},
	    {"kosovo", "XK"},
	    {"yemen", "YE"},
	    {"mayotte", "YT"},
	    {"south africa", "ZA"},
	    {"zambia", "ZM"},
	    {"zimbabwe", "ZW"},
	    {"cs", "CS"},
	    //this does not exist, but yahoo pretend we have revenue
	    {"serbia and montenegro", "CS"},
	    {"other", "US"}};
	return mappa;
}

QString getNationIsoCode(const QString& nation) {
	auto codes = getNationsIsoCodes();
	auto iter  = codes.find(nation.toLower());
	if (iter == codes.end()) {
		qCritical().noquote() << QSL("nation: %1 missing in getNationsIsoCodes \n").arg(nation.toLower()) + QStacker16Light();
		return QString();
	}
	return iter.value();
}
