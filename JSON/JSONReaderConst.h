#pragma once
#include <QString>
#include <limits>
namespace JSONReaderConst {
const std::string setMeSS     = "SET_ME";
const QString     setMe       = "SET_ME";
const QByteArray  setMe8      = "SET_ME";
constexpr int64_t setMeInt    = 0xBADC0FFEE0DDF00D;
constexpr double  setMeDouble = std::numeric_limits<double>::quiet_NaN();
} // namespace JSONReaderConst
