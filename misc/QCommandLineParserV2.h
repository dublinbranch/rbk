#ifndef QCOMMANDLINEPARSERV2_H
#define QCOMMANDLINEPARSERV2_H

#ifndef QBL
#define QBL(str) QByteArrayLiteral(str)
#define QSL(str) QStringLiteral(str)
#endif

#include <QCommandLineParser>

class QCommandLineParserV2 : public QCommandLineParser {
      public:
	QString require(const QString& key, const char *msg = nullptr);
};

#endif // QCOMMANDLINEPARSERV2_H
