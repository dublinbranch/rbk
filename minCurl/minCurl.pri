SOURCES += \
    $$PWD/errorlog.cpp \
    $$PWD/mincurl.cpp \
    $$PWD/curlpp.cpp \
    $$PWD/urlgetcontent.cpp

HEADERS += \
    $$PWD/errorlog.h \
    $$PWD/mincurl.h \
    $$PWD/curlpp.h \
    $$PWD/urlgetcontent.h

defined(withMailFetcher,var){
    SOURCES += $$PWD/mailfetcher.cpp
    HEADERS += $$PWD/mailfetcher.h
}

LIBS += -lcurl
