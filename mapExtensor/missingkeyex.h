#pragma once

#include "rbk/QStacker/exceptionv2.h"

class MissingKeyEX : public ExceptionV2 {
      public:
	MissingKeyEX(const QString& _msg)
	    : ExceptionV2(_msg, 6) {
		forcePrint = true;
	}
	MissingKeyEX(const std::string& _msg)
	    : ExceptionV2(_msg, 6) {
		forcePrint = true;
	}
};
