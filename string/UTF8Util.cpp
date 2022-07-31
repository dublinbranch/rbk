#include "UTF8Util.h"
#include <QTextCodec>

bool isValidUTF8(const std::string& string, QString* target) {
	return isValidUTF8(std::string_view(string), target);
}

bool isValidUTF8(std::string_view string, QString* target) {
	// https://stackoverflow.com/questions/18227530/check-if-utf-8-string-is-valid-in-qt
	QTextCodec::ConverterState state;
	auto                       codec = QTextCodec::codecForName("UTF-8");
	if (target) {
		*target = codec->toUnicode(string.data(), string.size(), &state);
	} else {
		codec->toUnicode(string.data(), string.size(), &state);
	}

	bool validUtf8 = state.invalidChars == 0;
	return validUtf8;
}
