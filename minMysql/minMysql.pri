LIBS += -lmariadb

CONFIG += object_parallel_to_source

HEADERS += \
    $$PWD/ClickHouseException.h \
	$$PWD/MITLS.h \
    $$PWD/clickhouse.h \
    $$PWD/min_mysql.h  \
    $$PWD/ttlcache.h \
	$$PWD/utilityfunctions.h
    
SOURCES += \
    $$PWD/clickhouse.cpp \
    $$PWD/min_mysql.cpp \
    $$PWD/ttlcache.cpp \
     \
    $$PWD/utilityfunctions.cpp
DISTFILES += /
	$$PWD/README.md 
