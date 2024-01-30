#include "b64.h"
#include "rbk/defines/stringDefine.h"
#include <QCryptographicHash>

QString base64this(const char* param) {
	// no alloc o.O
	QByteArray cheap;
	cheap.setRawData(param, (uint)strlen(param));
	return base64this(cheap);
}

QByteArray toBase64(const QByteViewV2& url64, bool urlSafe) {
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
