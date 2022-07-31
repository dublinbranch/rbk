HEADERS += \
	$$PWD/qdateship.h \
	$$PWD/qstringship.h

DISTFILES += /
	$$PWD/README.md

SOURCES += \
	$$PWD/implementation.cpp 
 
defined(HAS_QT_NETWORK, var) {
	HEADERS += $$PWD/qhostaddress.h 
	SOURCES += $$PWD/qhostaddress.cpp 
}
