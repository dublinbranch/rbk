#ifndef CLICKHOUSEEXCEPTION_H
#define CLICKHOUSEEXCEPTION_H

#include "rbk/QStacker/exceptionv2.h"
class ClickHouseException : public ExceptionV2 {
      public:
	ClickHouseException(const QString& _msg, uint skip = 4);
};

#endif // CLICKHOUSEEXCEPTION_H
