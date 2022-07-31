 SOURCES += \
    $$PWD/errorlog.cpp \
	$$PWD/mailfetcher.cpp \
    $$PWD/mincurl.cpp \
    $$PWD/curlpp.cpp \
    $$PWD/urlgetcontent.cpp 
    
HEADERS += \
	$$PWD/errorlog.h \
	$$PWD/mailfetcher.h \
    $$PWD/mincurl.h \
    $$PWD/curlpp.h \
    $$PWD/urlgetcontent.h 

LIBS += -lcurl
