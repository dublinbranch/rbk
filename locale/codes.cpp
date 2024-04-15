#include "codes.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/minMysql/runnable.h"
#include <QDebug>
#include <QMap>

extern Runnable runnable;

const mapV2<QByteArray, bool>& getNationISO2() {
	const static mapV2<QByteArray, bool> nations = {{
		{"AD", true},
		{"AE", true},
		{"AF", true},
		{"AG", true},
		{"AI", true},
		{"AL", true},
		{"AM", true},
		{"AO", true},
		{"AQ", true},
		{"AR", true},
		{"AS", true},
		{"AT", true},
		{"AU", true},
		{"AW", true},
		{"AX", true},
		{"AZ", true},
		{"BA", true},
		{"BB", true},
		{"BD", true},
		{"BE", true},
		{"BF", true},
		{"BG", true},
		{"BH", true},
		{"BI", true},
		{"BJ", true},
		{"BL", true},
		{"BM", true},
		{"BN", true},
		{"BO", true},
		{"BQ", true},
		{"BR", true},
		{"BS", true},
		{"BT", true},
		{"BV", true},
		{"BW", true},
		{"BY", true},
		{"BZ", true},
		{"CA", true},
		{"CC", true},
		{"CD", true},
		{"CF", true},
		{"CG", true},
		{"CH", true},
		{"CI", true},
		{"CK", true},
		{"CL", true},
		{"CM", true},
		{"CN", true},
		{"CO", true},
		{"CR", true},
		{"CU", true},
		{"CV", true},
		{"CW", true},
		{"CX", true},
		{"CY", true},
		{"CZ", true},
		{"DE", true},
		{"DJ", true},
		{"DK", true},
		{"DM", true},
		{"DO", true},
		{"DZ", true},
		{"EC", true},
		{"EE", true},
		{"EG", true},
		{"EH", true},
		{"ER", true},
		{"ES", true},
		{"ET", true},
		{"FI", true},
		{"FJ", true},
		{"FK", true},
		{"FM", true},
		{"FO", true},
		{"FR", true},
		{"GA", true},
		{"GB", true},
		{"GD", true},
		{"GE", true},
		{"GF", true},
		{"GG", true},
		{"GH", true},
		{"GI", true},
		{"GL", true},
		{"GM", true},
		{"GN", true},
		{"GP", true},
		{"GQ", true},
		{"GR", true},
		{"GS", true},
		{"GT", true},
		{"GU", true},
		{"GW", true},
		{"GY", true},
		{"HK", true},
		{"HM", true},
		{"HN", true},
		{"HR", true},
		{"HT", true},
		{"HU", true},
		{"ID", true},
		{"IE", true},
		{"IL", true},
		{"IM", true},
		{"IN", true},
		{"IO", true},
		{"IQ", true},
		{"IR", true},
		{"IS", true},
		{"IT", true},
		{"JE", true},
		{"JM", true},
		{"JO", true},
		{"JP", true},
		{"KE", true},
		{"KG", true},
		{"KH", true},
		{"KI", true},
		{"KM", true},
		{"KN", true},
		{"KP", true},
		{"KR", true},
		{"KW", true},
		{"KY", true},
		{"KZ", true},
		{"LA", true},
		{"LB", true},
		{"LC", true},
		{"LI", true},
		{"LK", true},
		{"LR", true},
		{"LS", true},
		{"LT", true},
		{"LU", true},
		{"LV", true},
		{"LY", true},
		{"MA", true},
		{"MC", true},
		{"MD", true},
		{"ME", true},
		{"MF", true},
		{"MG", true},
		{"MH", true},
		{"MK", true},
		{"ML", true},
		{"MM", true},
		{"MN", true},
		{"MO", true},
		{"MP", true},
		{"MQ", true},
		{"MR", true},
		{"MS", true},
		{"MT", true},
		{"MU", true},
		{"MV", true},
		{"MW", true},
		{"MX", true},
		{"MY", true},
		{"MZ", true},
		{"NA", true},
		{"NC", true},
		{"NE", true},
		{"NF", true},
		{"NG", true},
		{"NI", true},
		{"NL", true},
		{"NO", true},
		{"NP", true},
		{"NR", true},
		{"NU", true},
		{"NZ", true},
		{"OM", true},
		{"PA", true},
		{"PE", true},
		{"PF", true},
		{"PG", true},
		{"PH", true},
		{"PK", true},
		{"PL", true},
		{"PM", true},
		{"PN", true},
		{"PR", true},
		{"PS", true},
		{"PT", true},
		{"PW", true},
		{"PY", true},
		{"QA", true},
		{"RE", true},
		{"RO", true},
		{"RS", true},
		{"RU", true},
		{"RW", true},
		{"SA", true},
		{"SB", true},
		{"SC", true},
		{"SD", true},
		{"SE", true},
		{"SG", true},
		{"SH", true},
		{"SI", true},
		{"SJ", true},
		{"SK", true},
		{"SL", true},
		{"SM", true},
		{"SN", true},
		{"SO", true},
		{"SR", true},
		{"SS", true},
		{"ST", true},
		{"SV", true},
		{"SX", true},
		{"SY", true},
		{"SZ", true},
		{"TC", true},
		{"TD", true},
		{"TF", true},
		{"TG", true},
		{"TH", true},
		{"TJ", true},
		{"TK", true},
		{"TL", true},
		{"TM", true},
		{"TN", true},
		{"TO", true},
		{"TR", true},
		{"TT", true},
		{"TV", true},
		{"TW", true},
		{"TZ", true},
		{"UA", true},
		{"UG", true},
		{"UM", true},
		{"US", true},
		{"UY", true},
		{"UZ", true},
		{"VA", true},
		{"VC", true},
		{"VE", true},
		{"VG", true},
		{"VI", true},
		{"VN", true},
		{"VU", true},
		{"WF", true},
		{"WS", true},
		{"YE", true},
		{"YT", true},
		{"ZA", true},
		{"ZM", true},
		{"ZW", true},
	}};

	return nations;
}

const mapV2<QByteArray, quint16>& getNationISO2Adapted() {
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
			{"BEL", 20},
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
		}};

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
	    {"st. lucia", "LC"},
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
	    {"myanmar (burma)", "MM"},
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

QString getNationIsoCode(const QString& nation, const QString def) {
	auto codes = getNationsIsoCodes();
	auto iter  = codes.find(nation.toLower());
	if (iter == codes.end()) {
		if (def != "") {
			return def;
		}
		auto key = QSL("nation: %1 missing in getNationsIsoCodes \n").arg(nation.toLower());
		if (runnable(key, 3600)) {
			qCritical().noquote() << key + QStacker16Light();
		}
		return QString();
	}
	return iter.value();
}

bool validNation(const QByteArray& nation, bool okLatam) {
	if (okLatam && nation == "LATAM") {
		return true;
	}
	return getNationISO2Adapted().contains(nation);
}

const mapV2<QByteArray, QByteArray>& getNationISO3() {
	const static mapV2<QByteArray, QByteArray> nations = {
		{"AFG", "Afghanistan"},
		{"ALA", "Aland Islands"},
		{"ALB", "Albania"},
		{"DZA", "Algeria"},
		{"ASM", "American Samoa"},
		{"AND", "Andorra"},
		{"AGO", "Angola"},
		{"AIA", "Anguilla"},
		{"ATA", "Antarctica"},
		{"ATG", "Antigua and Barbuda"},
		{"ARG", "Argentina"},
		{"ARM", "Armenia"},
		{"ABW", "Aruba"},
		{"AUS", "Australia"},
		{"AUT", "Austria"},
		{"AZE", "Azerbaijan"},
		{"BHS", "Bahamas"},
		{"BHR", "Bahrain"},
		{"BGD", "Bangladesh"},
		{"BRB", "Barbados"},
		{"BLR", "Belarus"},
		{"BEL", "Belgium"},
		{"BLZ", "Belize"},
		{"BEN", "Benin"},
		{"BMU", "Bermuda"},
		{"BTN", "Bhutan"},
		{"BOL", "Bolivia"},
		{"BES", "Bonaire, Sint Eustatius and Saba"},
		{"BIH", "Bosnia and Herzegovina"},
		{"BWA", "Botswana"},
		{"BVT", "Bouvet Island"},
		{"BRA", "Brazil"},
		{"IOT", "British Indian Ocean Territory"},
		{"BRN", "Brunei Darussalam"},
		{"BGR", "Bulgaria"},
		{"BFA", "Burkina Faso"},
		{"BDI", "Burundi"},
		{"KHM", "Cambodia"},
		{"CMR", "Cameroon"},
		{"CAN", "Canada"},
		{"CPV", "Cape Verde"},
		{"CYM", "Cayman Islands"},
		{"CAF", "Central African Republic"},
		{"TCD", "Chad"},
		{"CHL", "Chile"},
		{"CHN", "China"},
		{"CXR", "Christmas Island"},
		{"CCK", "Cocos (Keeling) Islands"},
		{"COL", "Colombia"},
		{"COM", "Comoros"},
		{"COG", "Congo"},
		{"COD", "Congo, The Democratic Republic of "},
		{"COK", "Cook Islands"},
		{"CRI", "Costa Rica"},
		{"CIV", "Cote d'Ivoire"},
		{"HRV", "Croatia"},
		{"CUB", "Cuba"},
		{"CUW", "Curaçao"},
		{"CYP", "Cyprus"},
		{"CZE", "Czechia"},
		{"DNK", "Denmark"},
		{"DJI", "Djibouti"},
		{"DMA", "Dominica"},
		{"DOM", "Dominican Republic"},
		{"ECU", "Ecuador"},
		{"EGY", "Egypt"},
		{"SLV", "El Salvador"},
		{"GNQ", "Equatorial Guinea"},
		{"ERI", "Eritrea"},
		{"EST", "Estonia"},
		{"ETH", "Ethiopia"},
		{"FLK", "Falkland Islands (Malvinas)"},
		{"FRO", "Faroe Islands"},
		{"FJI", "Fiji"},
		{"FIN", "Finland"},
		{"FRA", "France"},
		{"GUF", "French Guiana"},
		{"PYF", "French Polynesia"},
		{"ATF", "French Southern Territories"},
		{"GAB", "Gabon"},
		{"GMB", "Gambia"},
		{"GEO", "Georgia"},
		{"DEU", "Germany"},
		{"GHA", "Ghana"},
		{"GIB", "Gibraltar"},
		{"GRC", "Greece"},
		{"GRL", "Greenland"},
		{"GRD", "Grenada"},
		{"GLP", "Guadeloupe"},
		{"GUM", "Guam"},
		{"GTM", "Guatemala"},
		{"GGY", "Guernsey"},
		{"GIN", "Guinea"},
		{"GNB", "Guinea-Bissau"},
		{"GUY", "Guyana"},
		{"HTI", "Haiti"},
		{"HMD", "Heard and Mc Donald Islands"},
		{"VAT", "Holy See (Vatican City State)"},
		{"HND", "Honduras"},
		{"HKG", "Hong Kong"},
		{"HUN", "Hungary"},
		{"ISL", "Iceland"},
		{"IND", "India"},
		{"IDN", "Indonesia"},
		{"IRN", "Iran, Islamic Republic of"},
		{"IRQ", "Iraq"},
		{"IRL", "Ireland"},
		{"IMN", "Isle of Man"},
		{"ISR", "Israel"},
		{"ITA", "Italy"},
		{"JAM", "Jamaica"},
		{"JPN", "Japan"},
		{"JEY", "Jersey"},
		{"JOR", "Jordan"},
		{"KAZ", "Kazakstan"},
		{"KEN", "Kenya"},
		{"KIR", "Kiribati"},
		{"PRK", "Korea, Democratic People's Republic of"},
		{"KOR", "Korea, Republic of"},
		{"XKX", "Kosovo (temporary code)"},
		{"KWT", "Kuwait"},
		{"KGZ", "Kyrgyzstan"},
		{"LAO", "Lao, People's Democratic Republic"},
		{"LVA", "Latvia"},
		{"LBN", "Lebanon"},
		{"LSO", "Lesotho"},
		{"LBR", "Liberia"},
		{"LBY", "Libyan Arab Jamahiriya"},
		{"LIE", "Liechtenstein"},
		{"LTU", "Lithuania"},
		{"LUX", "Luxembourg"},
		{"MAC", "Macao"},
		{"MKD", "Macedonia, The Former Yugoslav Republic Of"},
		{"MDG", "Madagascar"},
		{"MWI", "Malawi"},
		{"MYS", "Malaysia"},
		{"MDV", "Maldives"},
		{"MLI", "Mali"},
		{"MLT", "Malta"},
		{"MHL", "Marshall Islands"},
		{"MTQ", "Martinique"},
		{"MRT", "Mauritania"},
		{"MUS", "Mauritius"},
		{"MYT", "Mayotte"},
		{"MEX", "Mexico"},
		{"FSM", "Micronesia, Federated States of"},
		{"MDA", "Moldova, Republic of"},
		{"MCO", "Monaco"},
		{"MNG", "Mongolia"},
		{"MNE", "Montenegro"},
		{"MSR", "Montserrat"},
		{"MAR", "Morocco"},
		{"MOZ", "Mozambique"},
		{"MMR", "Myanmar"},
		{"NAM", "Namibia"},
		{"NRU", "Nauru"},
		{"NPL", "Nepal"},
		{"NLD", "Netherlands"},
		{"NCL", "New Caledonia"},
		{"NZL", "New Zealand"},
		{"NIC", "Nicaragua"},
		{"NER", "Niger"},
		{"NGA", "Nigeria"},
		{"NIU", "Niue"},
		{"NFK", "Norfolk Island"},
		{"MNP", "Northern Mariana Islands"},
		{"NOR", "Norway"},
		{"OMN", "Oman"},
		{"PAK", "Pakistan"},
		{"PLW", "Palau"},
		{"PSE", "Palestinian Territory, Occupied"},
		{"PAN", "Panama"},
		{"PNG", "Papua New Guinea"},
		{"PRY", "Paraguay"},
		{"PER", "Peru"},
		{"PHL", "Philippines"},
		{"PCN", "Pitcairn"},
		{"POL", "Poland"},
		{"PRT", "Portugal"},
		{"PRI", "Puerto Rico"},
		{"QAT", "Qatar"},
		{"SRB", "Republic of Serbia"},
		{"REU", "Reunion"},
		{"ROU", "Romania"},
		{"RUS", "Russia Federation"},
		{"RWA", "Rwanda"},
		{"BLM", "Saint Barthélemy"},
		{"SHN", "Saint Helena"},
		{"KNA", "Saint Kitts & Nevis"},
		{"LCA", "Saint Lucia"},
		{"MAF", "Saint Martin"},
		{"SPM", "Saint Pierre and Miquelon"},
		{"VCT", "Saint Vincent and the Grenadines"},
		{"WSM", "Samoa"},
		{"SMR", "San Marino"},
		{"STP", "Sao Tome and Principe"},
		{"SAU", "Saudi Arabia"},
		{"SEN", "Senegal"},
		{"SYC", "Seychelles"},
		{"SLE", "Sierra Leone"},
		{"SGP", "Singapore"},
		{"SXM", "Sint Maarten"},
		{"SVK", "Slovakia"},
		{"SVN", "Slovenia"},
		{"SLB", "Solomon Islands"},
		{"SOM", "Somalia"},
		{"ZAF", "South Africa"},
		{"SGS", "South Georgia & The South Sandwich Islands"},
		{"SSD", "South Sudan"},
		{"ESP", "Spain"},
		{"LKA", "Sri Lanka"},
		{"SDN", "Sudan"},
		{"SUR", "Suriname"},
		{"SJM", "Svalbard and Jan Mayen"},
		{"SWZ", "Swaziland"},
		{"SWE", "Sweden"},
		{"CHE", "Switzerland"},
		{"SYR", "Syrian Arab Republic"},
		{"TWN", "Taiwan, Province of China"},
		{"TJK", "Tajikistan"},
		{"TZA", "Tanzania, United Republic of"},
		{"THA", "Thailand"},
		{"TLS", "Timor-Leste"},
		{"TGO", "Togo"},
		{"TKL", "Tokelau"},
		{"TON", "Tonga"},
		{"TTO", "Trinidad and Tobago"},
		{"TUN", "Tunisia"},
		{"TUR", "Turkey"},
		{"XTX", "Turkish Rep N Cyprus (temporary code)"},
		{"TKM", "Turkmenistan"},
		{"TCA", "Turks and Caicos Islands"},
		{"TUV", "Tuvalu"},
		{"UGA", "Uganda"},
		{"UKR", "Ukraine"},
		{"ARE", "United Arab Emirates"},
		{"GBR", "United Kingdom"},
		{"USA", "United States"},
		{"UMI", "United States Minor Outlying Islands"},
		{"URY", "Uruguay"},
		{"UZB", "Uzbekistan"},
		{"VUT", "Vanuatu"},
		{"VEN", "Venezuela"},
		{"VNM", "Vietnam"},
		{"VGB", "Virgin Islands, British"},
		{"VIR", "Virgin Islands, U.S."},
		{"WLF", "Wallis and Futuna"},
		{"ESH", "Western Sahara"},
		{"YEM", "Yemen"},
		{"ZMB", "Zambia"},
		{"ZWE", "Zimbabwe"},
	};
	return nations;
}

const mapV2<QByteArray, QByteArray>& getNationISO2_V2() {
	const static mapV2<QByteArray, QByteArray> nations =
		{
			{"AD", "AD"},
			{"AE", "AE"},
			{"UAE", "AE"},
			{"AF", "AF"},
			{"AG", "AG"},
			{"AI", "AI"},
			{"AL", "AL"},
			{"AM", "AM"},
			{"AO", "AO"},
			{"AQ", "AQ"},
			{"AR", "AR"},
			{"AS", "AS"},
			{"AT", "AT"},
			{"AU", "AU"},
			{"AW", "AW"},
			{"AX", "AX"},
			{"AZ", "AZ"},
			{"BA", "BA"},
			{"BB", "BB"},
			{"BD", "BD"},
			{"BE", "BE"},
			{"BEL", "BE"},
			{"BF", "BF"},
			{"BG", "BG"},
			{"BH", "BH"},
			{"BI", "BI"},
			{"BJ", "BJ"},
			{"BL", "BL"},
			{"BM", "BM"},
			{"BN", "BN"},
			{"BO", "BO"},
			{"BQ", "BQ"},
			{"BR", "BR"},
			{"BS", "BS"},
			{"BT", "BT"},
			{"BV", "BV"},
			{"BW", "BW"},
			{"BY", "BY"},
			{"BZ", "BZ"},
			{"CA", "CA"},
			{"CC", "CC"},
			{"CD", "CD"},
			{"CF", "CF"},
			{"CG", "CG"},
			{"CH", "CH"},
			{"CI", "CI"},
			{"CK", "CK"},
			{"CL", "CL"},
			{"CM", "CM"},
			{"CN", "CN"},
			{"CO", "CO"},
			{"CR", "CR"},
			{"CU", "CU"},
			{"CV", "CV"},
			{"CW", "CW"},
			{"CX", "CX"},
			{"CY", "CY"},
			{"CZ", "CZ"},
			{"DE", "DE"},
			{"DJ", "DJ"},
			{"DK", "DK"},
			{"DM", "DM"},
			{"DO", "DO"},
			{"DZ", "DZ"},
			{"EC", "EC"},
			{"EE", "EE"},
			{"EG", "EG"},
			{"EH", "EH"},
			{"ER", "ER"},
			{"ES", "ES"},
			{"ET", "ET"},
			{"FI", "FI"},
			{"FJ", "FJ"},
			{"FK", "FK"},
			{"FM", "FM"},
			{"FO", "FO"},
			{"FR", "FR"},
			{"GA", "GA"},
			{"GB", "GB"}, //this is the geographic toponim of the main island
			{"UK", "UK"}, //UK this is the actual politic entity (which comprise GB and many more island ecc)
			{"GD", "GD"},
			{"GE", "GE"},
			{"GF", "GF"},
			{"GG", "GG"},
			{"GH", "GH"},
			{"GI", "GI"},
			{"GL", "GL"},
			{"GM", "GM"},
			{"GN", "GN"},
			{"GP", "GP"},
			{"GQ", "GQ"},
			{"GR", "GR"},
			{"GS", "GS"},
			{"GT", "GT"},
			{"GU", "GU"},
			{"GW", "GW"},
			{"GY", "GY"},
			{"HK", "HK"},
			{"HM", "HM"},
			{"HN", "HN"},
			{"HR", "HR"},
			{"HT", "HT"},
			{"HU", "HU"},
			{"ID", "ID"},
			{"IE", "IE"},
			{"IL", "IL"},
			{"IM", "IM"},
			{"IN", "IN"},
			{"IO", "IO"},
			{"IQ", "IQ"},
			{"IR", "IR"},
			{"IS", "IS"},
			{"IT", "IT"},
			{"JE", "JE"},
			{"JM", "JM"},
			{"JO", "JO"},
			{"JP", "JP"},
			{"KE", "KE"},
			{"KG", "KG"},
			{"KH", "KH"},
			{"KI", "KI"},
			{"KM", "KM"},
			{"KN", "KN"},
			{"KP", "KP"},
			{"KR", "KR"},
			{"KW", "KW"},
			{"KY", "KY"},
			{"KZ", "KZ"},
			{"LA", "LA"},
			{"LB", "LB"},
			{"LC", "LC"},
			{"LI", "LI"},
			{"LK", "LK"},
			{"LR", "LR"},
			{"LS", "LS"},
			{"LT", "LT"},
			{"LU", "LU"},
			{"LV", "LV"},
			{"LY", "LY"},
			{"MA", "MA"},
			{"MC", "MC"},
			{"MD", "MD"},
			{"ME", "ME"},
			{"MF", "MF"},
			{"MG", "MG"},
			{"MH", "MH"},
			{"MK", "MK"},
			{"ML", "ML"},
			{"MM", "MM"},
			{"MN", "MN"},
			{"MO", "MO"},
			{"MP", "MP"},
			{"MQ", "MQ"},
			{"MR", "MR"},
			{"MS", "MS"},
			{"MT", "MT"},
			{"MU", "MU"},
			{"MV", "MV"},
			{"MW", "MW"},
			{"MX", "MX"},
			{"MY", "MY"},
			{"MZ", "MZ"},
			{"NA", "NA"},
			{"NC", "NC"},
			{"NE", "NE"},
			{"NF", "NF"},
			{"NG", "NG"},
			{"NI", "NI"},
			{"NL", "NL"},
			{"NO", "NO"},
			{"NP", "NP"},
			{"NR", "NR"},
			{"NU", "NU"},
			{"NZ", "NZ"},
			{"OM", "OM"},
			{"PA", "PA"},
			{"PE", "PE"},
			{"PF", "PF"},
			{"PG", "PG"},
			{"PH", "PH"},
			{"PK", "PK"},
			{"PL", "PL"},
			{"PM", "PM"},
			{"PN", "PN"},
			{"PR", "PR"},
			{"PS", "PS"},
			{"PT", "PT"},
			{"PW", "PW"},
			{"PY", "PY"},
			{"QA", "QA"},
			{"RE", "RE"},
			{"RO", "RO"},
			{"RS", "RS"},
			{"RU", "RU"},
			{"RW", "RW"},
			{"SA", "SA"},
			{"SB", "SB"},
			{"SC", "SC"},
			{"SD", "SD"},
			{"SE", "SE"},
			{"SG", "SG"},
			{"SH", "SH"},
			{"SI", "SI"},
			{"SJ", "SJ"},
			{"SK", "SK"},
			{"SL", "SL"},
			{"SM", "SM"},
			{"SN", "SN"},
			{"SO", "SO"},
			{"SR", "SR"},
			{"SS", "SS"},
			{"ST", "ST"},
			{"SV", "SV"},
			{"SX", "SX"},
			{"SY", "SY"},
			{"SZ", "SZ"},
			{"TC", "TC"},
			{"TD", "TD"},
			{"TF", "TF"},
			{"TG", "TG"},
			{"TH", "TH"},
			{"TJ", "TJ"},
			{"TK", "TK"},
			{"TL", "TL"},
			{"TM", "TM"},
			{"TN", "TN"},
			{"TO", "TO"},
			{"TR", "TR"},
			{"TT", "TT"},
			{"TV", "TV"},
			{"TW", "TW"},
			{"TZ", "TZ"},
			{"UA", "UA"},
			{"UG", "UG"},
			{"UM", "UM"},
			{"US", "US"},
			{"UY", "UY"},
			{"UZ", "UZ"},
			{"VA", "VA"},
			{"VC", "VC"},
			{"VE", "VE"},
			{"VG", "VG"},
			{"VI", "VI"},
			{"VN", "VN"},
			{"VU", "VU"},
			{"WF", "WF"},
			{"WS", "WS"},
			{"YE", "YE"},
			{"YT", "YT"},
			{"ZA", "ZA"},
			{"ZM", "ZM"},
			{"ZW", "ZW"},
		};

	return nations;
}
