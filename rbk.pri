#this will enable certain ip based function on localeV2, of course you will need to provide a max mind db handler
#put those in PRO file of the project!
#DEFINES += localeWithMaxMind
#WITH_BOOST_BEAST = true

#In case you drop in a project who used the older folder structure, this can save some time (add in the config.pri)
#INCLUDEPATH += $$PWD/rbk/ 

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


#Still unsure how it improoves the debug, but it cripples mold so OUT!
#-ggdb3
QMAKE_CXXFLAGS += -Wunused -Wunused-function
QMAKE_CXXFLAGS += -std=gnu++2b
QMAKE_CXXFLAGS += -Wall -Wextra -Wshadow -Wshadow-local -Wshadow-compatible-local

CONFIG += object_parallel_to_source

DEFINES += GIT_STATUS='\\"$(shell git -C '$$_PRO_FILE_PWD_' describe  --always --dirty --abbrev=99)\\"'
DEFINES += COMPILATION_TIME='\\"$(shell TZ=UTC date +\"%Y-%m-%dT%T\")\\"'

#this should speed up the update of the submodule info
CONFIG += resources_big

#except slows down everything immensely -.- so when needed I will just bump before putting live
#usually only Roy leaves this one as is only relevant for live code
#DEFINES += SmolHack1=0'$(shell touch '$$PWD'/gitTrick/buffer.cpp)'

#this one need to be stored in a file as contain newline and other complex char, same stuff as above cache all!
#sometime, for some reason is not able to auto create the file, just touch rbk/gitTrick/submoduleInfo
system(git -C '$$_PRO_FILE_PWD_' submodule foreach git describe --always --abbrev=99 --dirty > '$$_PRO_FILE_PWD_'/rbk/gitTrick/submoduleInfo)
#DEFINES += SmolHack2=0'$(shell git -C '$$_PRO_FILE_PWD_' submodule foreach git describe --always --abbrev=99 --dirty > '$$_PRO_FILE_PWD_'/rbk/gitTrick/submoduleInfo)'

#QT is amazing, it can easily embedd and later read such file, there is not noticeable penalty in linking time for this operation
RESOURCES     = $$PWD/gitTrick/resources.qrc


# - SETUP -
# zypper in libzip-devel
# OR (should be equivalent)
LIBS += -lzip
#to execute
#zypper in libmaxminddb0 
#to develop libmaxminddb-devel
LIBS += -lmaxminddb
# if compile error because not found "maxminddb.h" file (included in GeoLite2PP.hpp) then install "libmaxminddb-devel" package in YaST2F
LIBS += -L'$$PWD/GeoLite2PP' -lgeolite2++


LIBS += -ldw
LIBS += -ldl
LIBS += -lfmt
LIBS += -lcurl
#zypper in libmariadb3
LIBS += -lmariadb

defined(WITH_BOOST_BEAST,var) {
#HTTP part
DEFINES += BOOST_BEAST_USE_STD_STRING_VIEW=1

SOURCES += \
    $$PWD/HTTP/PMFCGI.cpp \
    $$PWD/HTTP/beast.cpp \
    $$PWD/HTTP/router.cpp \
    $$PWD/HTTP/select2.cpp \
	
HEADERS += \
    $$PWD/HTTP/beastConfig.h \
    $$PWD/HTTP/select2.h \
    $$PWD/HTTP/PMFCGI.h \
    $$PWD/HTTP/Payload.h \
    $$PWD/HTTP/beast.h \
    $$PWD/HTTP/router.h \
}

DISTFILES += \
	$$PWD/GeoLite2PP/README.md \
	$$PWD/GeoLite2PP/libgeolite2++.a \
	$$PWD/JSON/LICENSE \
	$$PWD/JSON/README.md \
	$$PWD/SpaceShipOP/LICENSE.md \
	$$PWD/SpaceShipOP/README.md \
	$$PWD/SpaceShipOP/qspaceship.pri \
	$$PWD/minCurl/README.md \
	$$PWD/minMysql/README.md \
	$$PWD/minMysql/minMysql.pri

    
	
HEADERS += \
    $$PWD/BoostJson/extra.h \
    $$PWD/BoostJson/fwd.h \
    $$PWD/QStacker/CxaLevel.h \
    $$PWD/dateTime/util.h \
    $$PWD/fmtExtra/dynamic.h \
    $$PWD/fmtExtra/fromEnum.h \
    $$PWD/hash/salt.h \
    $$PWD/hash/sha.h \
	$$PWD/locale/codes.h \
    $$PWD/minMysql/DBConf.h \
    $$PWD/minMysql/runnable.h \
    $$PWD/minMysql/sqlRow.h \
    $$PWD/minMysql/sqlcomposer.h \
    $$PWD/minMysql/sqlresult.h \
    $$PWD/misc/echo.h \
    $$PWD/misc/ieee754.h \
    $$PWD/misc/swapType.h \
    $$PWD/misc/typename.h \
	$$PWD/rand/clampednormaldistribution.h \
    $$PWD/JSON/JSONReaderConst.h \
    $$PWD/JSON/jsonreader.h \
    $$PWD/SpaceShipOP/qdateship.h \
    $$PWD/SpaceShipOP/qstringship.h \
	$$PWD/caching/apcu2.h \
	$$PWD/dateTime/qdatetimev2.h \
    $$PWD/dateTime/timespecV2.h \
    $$PWD/GeoLite2PP/GeoLite2PP_error_category.hpp \
    $$PWD/RAII/resetAfterUse.h \
    $$PWD/defines/stringDefine.h \
    $$PWD/filesystem/ffCommon.h \
    $$PWD/filesystem/filefunction.h \
    $$PWD/filesystem/folder.h \
    $$PWD/fmtExtra/customformatter.h \
	$$PWD/fmtExtra/includeMe.h \
    $$PWD/gitTrick/buffer.h \
    $$PWD/hash/crc.h \
    $$PWD/locale/localev2.h \
    $$PWD/minCurl/curlpp.h \
    $$PWD/minCurl/errorlog.h \
    $$PWD/minCurl/mailfetcher.h \
    $$PWD/minCurl/mincurl.h \
    $$PWD/minCurl/qstringtokenizer.h \
    $$PWD/minCurl/urlgetcontent.h \
    $$PWD/misc/QCommandLineParserV2.h \
    $$PWD/misc/QDebugConfig.h \
    $$PWD/misc/QDebugHandler.h \
    $$PWD/misc/UaDecoder.h \
    $$PWD/misc/b64.h \
    $$PWD/misc/slacksender.h \
    $$PWD/misc/snowflake.h \
    $$PWD/misc/sourcelocation.h \
    $$PWD/misc/twilio.h \
    $$PWD/mixin/CopyAssignable.h \
    $$PWD/mixin/NoCopy.h \
    $$PWD/rand/randutil.h \
    $$PWD/serialization/QDataStreamer.h \
	$$PWD/serialization/asstring.h \
    $$PWD/serialization/serialize.h \
    $$PWD/string/UTF8Util.h \
    $$PWD/string/qstring.h \
    $$PWD/string/util.h \
    $$PWD/thread/threadstatush.h \
    $$PWD/thread/tmonitoring.h

SOURCES += \
    $$PWD/BoostJson/extra.cpp \
    $$PWD/HTTP/Payload.cpp \
	$$PWD/asanOption.cpp \
    $$PWD/dateTime/util.cpp \
    $$PWD/hash/salt.cpp \
    $$PWD/hash/sha.cpp \
	$$PWD/locale/codes.cpp \
    $$PWD/minMysql/runnable.cpp \
    $$PWD/minMysql/sqlcomposer.cpp \
    $$PWD/minMysql/sqlresult.cpp \
    $$PWD/misc/echo.cpp \
    $$PWD/misc/ieee754.cpp \
    $$PWD/misc/typename.cpp \
	$$PWD/rand/clampednormaldistribution.cpp \
    $$PWD/JSON/jsonreader.cpp \
    $$PWD/SpaceShipOP/implementation.cpp \
    $$PWD/caching/apcu2.cpp \
    $$PWD/caching/apcuTest.cpp \
    $$PWD/filesystem/filefunction.cpp \
    $$PWD/filesystem/folder.cpp \
    $$PWD/fmtExtra/customformatter.cpp \
    $$PWD/gitTrick/buffer.cpp \
    $$PWD/hash/crc.cpp \
    $$PWD/locale/localev2.cpp \
    $$PWD/minCurl/curlpp.cpp \
    $$PWD/minCurl/errorlog.cpp \
    $$PWD/minCurl/mailfetcher.cpp \
    $$PWD/minCurl/mincurl.cpp \
    $$PWD/minCurl/urlgetcontent.cpp \
    $$PWD/misc/QCommandLineParserV2.cpp \
    $$PWD/misc/QDebugHandler.cpp \
    $$PWD/misc/UaDecoder.cpp \
    $$PWD/misc/b64.cpp \
    $$PWD/misc/slacksender.cpp \
    $$PWD/misc/snowflake.cpp \
    $$PWD/misc/sourcelocation.cpp \
    $$PWD/misc/twilio.cpp \
    $$PWD/rand/randutil.cpp \
    $$PWD/serialization/QDataStreamer.cpp \
    $$PWD/string/UTF8Util.cpp \
    $$PWD/string/qstring.cpp \
    $$PWD/string/util.cpp \
    $$PWD/thread/threadstatush.cpp \
    $$PWD/thread/tmonitoring.cpp

SOURCES += $$PWD/HTTP/url.cpp 
	
HEADERS += $$PWD/HTTP/url.h 

SOURCES += \
	$$PWD/QStacker/exceptionv2.cpp \
	$$PWD/QStacker/httpexception.cpp \
    $$PWD/QStacker/qstacker.cpp 
    
HEADERS += \
    $$PWD/QStacker/backward.hpp \
    $$PWD/QStacker/exceptionv2.h \
    $$PWD/QStacker/httpexception.h \
    $$PWD/QStacker/qstacker.h 

INCLUDEPATH += $$PWD/rapidjson/

SOURCES += $$PWD/rapidjson/rapidAssert.cpp 
HEADERS += $$PWD/rapidjson/includeMe.h


HEADERS += \
	$$PWD/mapExtensor/RRList.h \
	$$PWD/mapExtensor/fixedSizeVector.h \
	$$PWD/mapExtensor/hmap.h \
	$$PWD/mapExtensor/indexedvector.h \
	$$PWD/mapExtensor/lockguardv2.h \
	$$PWD/mapExtensor/mapV2.h \
	$$PWD/mapExtensor/qmapV2.h  \
	$$PWD/mapExtensor/rwguard.h \
	$$PWD/mapExtensor/valueMitWarning.h \
	$$PWD/mapExtensor/vectorV2.h \
	$$PWD/mapExtensor/joinVector.h

DISTFILES += /
	$$PWD/mapExtensor/README.md

SOURCES += \
    $$PWD/mapExtensor/indexedvector.cpp \
	$$PWD/mapExtensor/lockguardv2.cpp \
    $$PWD/mapExtensor/mapV2.cpp \
    $$PWD/mapExtensor/rwguard.cpp
    
    
#for rapidJson
QMAKE_CXXFLAGS += -msse4.2


HEADERS += \
    $$PWD/JSON/safeGet.h \
    $$PWD/JSON/various.h 

SOURCES += \
    $$PWD/JSON/safeGet.cpp \
    $$PWD/JSON/various.cpp 

    
HEADERS += \
    $$PWD/magicEnum/BetterEnum.hpp \
    $$PWD/magicEnum/enum.h \
    $$PWD/magicEnum/magic_enum.hpp \
    $$PWD/magicEnum/templateEnum.h \
    $$PWD/magicEnum/magic_from_string.hpp
    
    
HEADERS += \
    $$PWD/minMysql/ClickHouseException.h \
	$$PWD/minMysql/MITLS.h \
    $$PWD/minMysql/clickhouse.h \
    $$PWD/minMysql/min_mysql.h  \
    $$PWD/minMysql/ttlcache.h \
	$$PWD/minMysql/utilityfunctions.h
    
SOURCES += \
    $$PWD/minMysql/clickhouse.cpp \
    $$PWD/minMysql/min_mysql.cpp \
    $$PWD/minMysql/ttlcache.cpp \
    $$PWD/minMysql/utilityfunctions.cpp
    
DISTFILES += /
	$$PWD/README.md 
	
	
DISTFILES += \
	$$PWD/README.md 
	
defined(HAS_QT_NETWORK, var) {
	HEADERS += $$PWD/qhostaddress.h 
	SOURCES += $$PWD/qhostaddress.cpp 
}


#Mustache
INCLUDEPATH += $$PWD/mustache/
SOURCES += \
    $$PWD/mustache/renderer.cpp

HEADERS += $$PWD/mustache/boost/mustache.hpp
