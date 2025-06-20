#include "b64.h"
#include "rbk/defines/stringDefine.h"
#include <QCryptographicHash>

QString base64this(const char* param) {
	// no alloc o.O
	QByteArray cheap;
	cheap.setRawData(param, (uint)strlen(param));
	return base64this(cheap);
}

QByteArray toBase64(const QByteAdt& url64, bool urlSafe) {
	auto b = QByteArray::Base64Option::Base64UrlEncoding | QByteArray::Base64Option::OmitTrailingEquals;
	if (urlSafe) {
		return url64.toBase64(b);
	}
	//Trailing = are totally useless, change my mind
	return url64.toBase64(QByteArray::Base64Option::OmitTrailingEquals);
}

QByteArray fromBase64(const QByteArray& url64, bool urlSafe) {
	auto b = QByteArray::Base64Option::Base64UrlEncoding;
	if (urlSafe) {
		return QByteArray::fromBase64(url64, b);
	}
	return QByteArray::fromBase64(url64);
}

QString fromBase64(const QString& url64, bool urlSafe) {
	return fromBase64(url64.toUtf8(), urlSafe);
}

// https://stackoverflow.com/questions/12094280/qt-decode-binary-sequence-from-base64
//bool isB64Valid(QString input, bool checkLength) {
//	if (checkLength and (input.length() % 4 != 0))
//		return false;

//	auto found1 = QRegExp("^[A-Za-z0-9+/]+$").indexIn(input, QRegExp::CaretAtZero);
//	auto found2 = QRegExp("^[A-Za-z0-9+/]+=$").indexIn(input, QRegExp::CaretAtZero);
//	auto found3 = QRegExp("^[A-Za-z0-9+/]+==$").indexIn(input, QRegExp::CaretAtZero);

//	auto cond1 = found1 == -1;
//	auto cond2 = found2 == -1;
//	auto cond3 = found3 == -1;

//	if (cond1 && cond2 && cond3)
//		return false;
//	return true;
//}

bool isB64Valid(const QByteArray& input, bool urlSafe) {
	QByteArray::FromBase64Result decoded;
	if (urlSafe) {
		decoded = QByteArray::fromBase64Encoding(input, QByteArray::Base64Option::AbortOnBase64DecodingErrors | QByteArray::Base64Option::Base64UrlEncoding);
	} else {
		decoded = QByteArray::fromBase64Encoding(input, QByteArray::Base64Option::AbortOnBase64DecodingErrors);
	}
	auto ok = decoded.decodingStatus == QByteArray::Base64DecodingStatus::Ok;
	return ok;
}

bool isB64Valid(const QString& input, bool urlSafe) {
	return isB64Valid(input.toUtf8(), urlSafe);
}

QString base64this(const QByteArray& param) {
	return QBL("FROM_BASE64('") + param.toBase64() + QBL("')");
}

QString base64this(const QString& param) {
	auto a = param.toUtf8().toBase64();
	return QBL("FROM_BASE64('") + a + QBL("')");
}

QString base64this(const std::string_view& param) {
	QByteArray cheap;
	cheap.setRawData(param.data(), (uint)param.size());
	return base64this(cheap);
}

QString base64this(const std::string& param) {
	return base64this(std::string_view(param));
}

QString mayBeBase64(const QString& original, bool emptyAsNull) {
	if (original == SQL_NULL) {
		return original;
	}

	if (original.isEmpty()) {
		if (emptyAsNull) {
			return SQL_NULL;
		}
		return QSL("''");
	}

	return base64this(original);
}

QString base64Nullable(const QString& param, bool emptyAsNull) {
	return mayBeBase64(param, emptyAsNull);
}
QString base64Nullable4Where(const QString& param, bool emptyAsNull) {
	auto val = mayBeBase64(param, emptyAsNull);
	if (val == SQL_NULL) {
		return " IS NULL ";
	}
	return " = " + val;
}

QByteArray shortMd5(const QByteArray& string, bool hex) {
	auto hashed = QCryptographicHash::hash((string), QCryptographicHash::Md5);
	if (hex) {

	} else {
	}
	// take first 8 bytes (16 hexadecimal digits)
	auto truncated = hashed.left(16);
	return truncated;
}

QByteArray shortMd5(const QString& string, bool hex) {
	return shortMd5(string.toUtf8(), hex);
}

// quite slow, but is the only thing working, the trick of decoding re-encoding looks like can fail too, missing padding, invalid termination char and other edge cases
/**
 * @brief isValidBase64
 * @param coded
 * @param message
 * @return
 */
bool isValidBase64(QString coded, QString* message) {
	auto raw = coded.toUtf8();
	// https://en.wikipedia.org/wiki/ASCII
	/*
	BIN         OCT D   H
	010 0000	040	32	20	 space
	010 0001	041	33	21	!
	010 0010	042	34	22	"
	010 0011	043	35	23	#
	010 0100	044	36	24	$
	010 0101	045	37	25	%
	010 0110	046	38	26	&
	010 0111	047	39	27	'
	010 1000	050	40	28	(
	010 1001	051	41	29	)
	010 1010	052	42	2A	*
	010 1011	053	43	2B	+
	010 1100	054	44	2C	,
	010 1101	055	45	2D	-
	010 1110	056	46	2E	.
	010 1111	057	47	2F	/
	011 0000	060	48	30	0
	011 0001	061	49	31	1
	011 0010	062	50	32	2
	011 0011	063	51	33	3
	011 0100	064	52	34	4
	011 0101	065	53	35	5
	011 0110	066	54	36	6
	011 0111	067	55	37	7
	011 1000	070	56	38	8
	011 1001	071	57	39	9
	011 1010	072	58	3A	:
	011 1011	073	59	3B	;
	011 1100	074	60	3C	<
	011 1101	075	61	3D	= //padding
	011 1110	076	62	3E	>
	011 1111	077	63	3F	?
	100 0000	100	64	40	@
	100 0001	101	65	41	A
	100 0010	102	66	42	B
	100 0011	103	67	43	C
	100 0100	104	68	44	D
	100 0101	105	69	45	E
	100 0110	106	70	46	F
	100 0111	107	71	47	G
	100 1000	110	72	48	H
	100 1001	111	73	49	I
	100 1010	112	74	4A	J
	100 1011	113	75	4B	K
	100 1100	114	76	4C	L
	100 1101	115	77	4D	M
	100 1110	116	78	4E	N
	100 1111	117	79	4F	O
	101 0000	120	80	50	P
	101 0001	121	81	51	Q
	101 0010	122	82	52	R
	101 0011	123	83	53	S
	101 0100	124	84	54	T
	101 0101	125	85	55	U
	101 0110	126	86	56	V
	101 0111	127	87	57	W
	101 1000	130	88	58	X
	101 1001	131	89	59	Y
	101 1010	132	90	5A	Z
	101 1011	133	91	5B	[
	101 1100	134	92	5C	\	~	\
	101 1101	135	93	5D	]
	101 1110	136	94	5E	↑	^
	101 1111	137	95	5F	←	_
	110 0000	140	96	60		@	`
	110 0001	141	97	61		a
	110 0010	142	98	62		b
	110 0011	143	99	63		c
	110 0100	144	100	64		d
	110 0101	145	101	65		e
	110 0110	146	102	66		f
	110 0111	147	103	67		g
	110 1000	150	104	68		h
	110 1001	151	105	69		i
	110 1010	152	106	6A		j
	110 1011	153	107	6B		k
	110 1100	154	108	6C		l
	110 1101	155	109	6D		m
	110 1110	156	110	6E		n
	110 1111	157	111	6F		o
	111 0000	160	112	70		p
	111 0001	161	113	71		q
	111 0010	162	114	72		r
	111 0011	163	115	73		s
	111 0100	164	116	74		t
	111 0101	165	117	75		u
	111 0110	166	118	76		v
	111 0111	167	119	77		w
	111 1000	170	120	78		x
	111 1001	171	121	79		y
	111 1010	172	122	7A		z
	111 1011	173	123	7B		{
	111 1100	174	124	7C	ACK	¬	|
	111 1101	175	125	7D		}
	111 1110	176	126	7E	ESC	|	~
	     */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

	int pos = 0;
	for (auto& chr : raw) {
		switch (chr) {
		case 48 ... 57:  // number 0 ... 9
		case 64 ... 91:  // Upper case letter AZ
		case 96 ... 123: // Lower case letter az
		case '+':
		case '-':
		case '=':
		case '_':
		case '/':
			continue;
		default:
			if (message) {
				*message = QSL("invalid char at pos %1").arg(pos);
			}

			return false;
		}
	}
#pragma GCC diagnostic pop
	return true;
}
