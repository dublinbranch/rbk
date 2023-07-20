#include "exceptionv2.h"
#include "qstacker.h"
#include "rbk/misc/sourcelocation.h"
#include <QString>
#include <cxxabi.h>

ExceptionV2::ExceptionV2(const QString& _msg, uint skip) {
	msg = _msg.toStdString() + stacker(skip, QStackerOptLight);
}

ExceptionV2::ExceptionV2(const char* _msg) {
	msg = _msg + stacker(4, QStackerOptLight);
}

ExceptionV2::ExceptionV2(const char* _msg, uint skip) {
	msg = _msg + stacker(skip, QStackerOptLight);
}

ExceptionV2::ExceptionV2(const std::string& _msg, uint skip) {
	msg = _msg + stacker(skip, QStackerOptLight);
}

ExceptionV2::ExceptionV2(const QByteArray& _msg, uint skip) {
	msg = _msg.toStdString() + stacker(skip, QStackerOptLight);
}

ExceptionV2 ExceptionV2::raw(const std::string& _msg) {
	ExceptionV2 e;
	e.setMsg(_msg);
	return e;
}

ExceptionV2 ExceptionV2::location(const std::string& _msg, const sourceLocation location) {
	ExceptionV2 e;
	e.setMsg(_msg + " in " + locationFull(location));
	return e;
}

ExceptionV2 ExceptionV2::location(const QString& _msg, const sourceLocation location) {
	ExceptionV2 e;
	e.setMsg(_msg.toStdString() + " in " + locationFull(location));
	return e;
}

/// We must mark this function to be skipped by lib asan as we do this unsafe operation
/// const auto* v2 = static_cast<ExceptionV2*>(thrown_exception);
/// if (v2->canaryKey == ExceptionV2::uukey) {
/// which is in case of a non ExceptionV2 is an out of bound access!
/// https://clang.llvm.org/docs/AddressSanitizer.html#issue-suppression
__attribute__((no_sanitize("address"))) bool ExceptionV2::isExceptionV2Derived(void* ptr) {
	const auto* v2 = static_cast<ExceptionV2*>(ptr);
	return v2->canaryKey == ExceptionV2::uukey;
}

const std::string ExceptionV2::getLogFile() const noexcept {
	static const std::string addr = "ExceptionV2.log";
	return addr;
}

const char* ExceptionV2::what() const noexcept {
	return msg.data();
}

void ExceptionV2::setMsg(const QByteArray& newMsg) {
	msg = newMsg.toStdString();
}

void ExceptionV2::setMsg(const std::string& newMsg) {
	msg = newMsg;
}

const char* currentExceptionTypeName() {
	int status = 0;
	return abi::__cxa_demangle(abi::__cxa_current_exception_type()->name(), nullptr, 0, &status);
}
