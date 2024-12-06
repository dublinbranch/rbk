#git@github.com:dublinbranch/rbk.git
#this will enable certain ip based function on localeV2, of course you will need to provide a max mind db handler
#put those in PRO file of the project!
#DEFINES += WithMaxMind


#this will enable the boost beast part, it is a bit heavy, but it is the best for the job
#note the different sintax, as this is only inside qmake
#WITH_BOOST_BEAST = true

#In case you drop in a project who used the older folder structure, this can save some time (add in the config.pri)
#INCLUDEPATH += $$PWD/rbk/ 

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# https://stackoverflow.com/a/30536286/1040618F
# the line below suppresses warnings generated by Qt's header files: we tell
# GCC to treat Qt's headers as "system headers" with the -isystem flag
QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]/QtCore


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


#Still unsure how it improoves the debug, but it cripples mold so OUT!
#-ggdb3
QMAKE_CXXFLAGS += -Wunused -Wunused-function 
#CONFIG += c++2b
QMAKE_CXXFLAGS += -std=gnu++2b
#-Werror -Wconversion
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic -Wshadow -Wshadow-local -Wshadow-compatible-local -Wconversion -fno-permissive -Werror=return-type

CONFIG += object_parallel_to_source
linux {
    DEFINES += GIT_STATUS='\\"$(shell git -C '$$_PRO_FILE_PWD_' describe  --always --dirty --abbrev=99)\\"'

    #except slows down everything immensely -.- so when needed I will just bump before putting live
    #usually only Roy leaves this one as is only relevant for live code
    #DEFINES += SmolHack1=0'$(shell touch '$$PWD'/gitTrick/buffer.cpp)'

    #this one need to be stored in a file as contain newline and other complex char, same stuff as above cache all!
    #sometime, for some reason is not able to auto create the file, just touch rbk/gitTrick/submoduleInfo
    system(git -C '$$_PRO_FILE_PWD_' submodule foreach git describe --always --abbrev=99 --dirty > '$$_PRO_FILE_PWD_'/rbk/gitTrick/submoduleInfo)
    #DEFINES += SmolHack2=0'$(shell git -C '$$_PRO_FILE_PWD_' submodule foreach git describe --always --abbrev=99 --dirty > '$$_PRO_FILE_PWD_'/rbk/gitTrick/submoduleInfo)'

    #QT is amazing, it can easily embedd and later read such file, there is not noticeable penalty in linking time for this operation
    RESOURCES     += $$PWD/gitTrick/resources.qrc

    #great control on memory and overall just better
    #zypper in jemalloc-devel
    LIBS += -ljemalloc


    #zypper in libdw-devel
    LIBS += -ldw
    LIBS += -ldl
    LIBS += -lfmt   #zypper in fmt-devel should be enought

    #zypper in libmariadb3 libmariadb-devel
    LIBS += -lmariadb
}
win32{
#win equivalent of ldw
    LIBS += -ldbghelp
}

#needed for dynamic creation of file into the main source directory
DEFINES += BasePath='\\"$$_PRO_FILE_PWD_\\"'

#this should speed up the update of the submodule info
CONFIG += resources_big


SOURCES += \
    $$PWD/QR/qr_code.cpp \
    $$PWD/QR/qrcodegen.cpp \
    $$PWD/minMysql/sqlrowv2.cpp \
    $$PWD/HTTP/PMFCGI.cpp \
    $$PWD/caching/cachable.cpp \
    $$PWD/dateTime/timerange.cpp \
    $$PWD/filesystem/suffix.cpp \
    $$PWD/hash/string.cpp \
    $$PWD/log/log.cpp \
    $$PWD/mapExtensor/ankerv2.cpp \
    $$PWD/mapExtensor/missingkeyex.cpp \
    $$PWD/minMysql/rowswap.cpp \
    $$PWD/minMysql/sqlbuffering.cpp \
    $$PWD/misc/base32.cpp \
    $$PWD/misc/qelapsedtimerv2.cpp \
    $$PWD/misc/sleep.cpp \
    $$PWD/string/qstringview.cpp \
    $$PWD/HTTP/mime.cpp \
    $$PWD/HTTP/util.cpp \
    $$PWD/string/comparator.cpp   \
    $$PWD/number/doubleoperator.cpp \
    $$PWD/string/stringoso.cpp

HEADERS += \
    $$PWD/QR/qr_code.h \
    $$PWD/QR/qrcodegen.hpp \
    $$PWD/minMysql/sqlrowv2.h \
	$$PWD/BoostJson/tagInvokeCrono.h \
    $$PWD/BoostJson/taginvoke.h \
    $$PWD/BoostJson/array.h \
	$$PWD/concept/concepts.h \
	$$PWD/concept/isSharedPtr.h \
    $$PWD/HTTP/PMFCGI.h \
    $$PWD/caching/cachable.h \
    $$PWD/filesystem/suffix.h \
    $$PWD/hash/rapidhash.h \
    $$PWD/hash/string.h \
    $$PWD/log/log.h \
    $$PWD/mapExtensor/ThreadSafeMultiIndex.hpp \
    $$PWD/mapExtensor/ankerl_unordered_dense.h \
    $$PWD/mapExtensor/ankerv2.h \
    $$PWD/mapExtensor/missingkeyex.h \
    $$PWD/minMysql/checkSchema/CheckSchemaConf.h \
    $$PWD/minMysql/checkSchema/CheckSchemaDescribe.h \
    $$PWD/minMysql/rowSwappable.h \
    $$PWD/minMysql/rowswap.h \
    $$PWD/minMysql/sqlbuffering.h \
	$$PWD/misc/NanoSpammerConfigDescribe.h \
    $$PWD/misc/base32.h \
    $$PWD/misc/controlFlowMacro.h \
    $$PWD/misc/decimate.h \
    $$PWD/misc/qelapsedtimerv2.h \
    $$PWD/misc/sleep.h \
    $$PWD/number/intTypes.h \
    $$PWD/string/qstringview.h \
    $$PWD/string/stringoso.h \
    $$PWD/types/isOptional.h \
    $$PWD/HTTP/mime.h \
	 $$PWD/HTTP/util.h \
    $$PWD/number/doubleoperator.h \
    $$PWD/string/comparator.h 

defined(WITH_BoostMysql,var){
SOURCES += \
    $$PWD/BoostMysql/includeme.cpp
    
HEADERS += \
    $$PWD/BoostMysql/includeme.h
}

defined(WITH_SODIUM,var){
#zypper in sodium-devel
LIBS += -lsodium

HEADERS += \
    $$PWD/Sodium/crypto.h
    
SOURCES += \
    $$PWD/Sodium/crypto.cpp
}

defined(WITH_SSL,var){
#zypper in libopenssl-devel
LIBS += -lssl -lcrypto

HEADERS += \
    $$PWD/totp/totp.h
    
SOURCES += \
    $$PWD/totp/totp.cpp
}

defined(WITH_REPROC,var) {
#for the external process invocation, for *REASON* the full path is needed
#zypper addrepo https://download.opensuse.org/repositories/devel:libraries:c_c++/openSUSE_Factory_PowerPC/devel:libraries:c_c++.repo
#zypper refresh
#zypper install libreproc14 libreproc++14 reproc-devel
#THE ORDER IS IMPORTANT!!!
LIBS +=  -lreproc++ -lreproc

SOURCES += \
   $$PWD/misc/executor.cpp
   
HEADERS += \
    $$PWD/misc/executor.h
    
}


defined(WITH_ZIPPER,var) {
# - SETUP -
# zypper in libzip-devel
# OR (should be equivalent)
LIBS += -lzip

SOURCES += \
    $$PWD/misc/zip.cpp
HEADERS += \
    $$PWD/misc/zip.h
}
    
defined(WITH_BOOST_BEAST,var) {
#HTTP part that trigger boost beast stuff

SOURCES += \
    $$PWD/HTTP/beast.cpp \
    $$PWD/HTTP/beastConfig.cpp \
    $$PWD/HTTP/router.cpp \
    $$PWD/HTTP/select2.cpp
	
HEADERS += \
    $$PWD/HTTP/beastConfig.h \
    $$PWD/HTTP/select2.h \
    $$PWD/HTTP/Payload.h \
    $$PWD/HTTP/beast.h \
    $$PWD/HTTP/router.h
}

defined(WithMaxMind,var) {
#zypper in libmaxminddb0
#to develop libmaxminddb-devel
LIBS += -lmaxminddb
# if compile error because not found "maxminddb.h" file (included in GeoLite2PP.hpp) then install "libmaxminddb-devel" package in YaST2F
LIBS += -L'$$PWD/GeoLite2PP' -lgeolite2++

DISTFILES += \
        $$PWD/GeoLite2PP/README.md \
        $$PWD/GeoLite2PP/libgeolite2++.a \
HEADERS += \
    $$PWD/GeoLite2PP/GeoLite2PP_error_category.hpp

}
#End with maxmind


defined(withMinCurl,var){

    DEFINES += useMinCurl

    #zypper in libcurl-devel
    LIBS += -lcurl

    HEADERS += \
        $$PWD/minCurl/curlpp.h \
        $$PWD/minCurl/errorlog.h \
        $$PWD/minCurl/mailfetcher.h \
        $$PWD/minCurl/mincurl.h \
        $$PWD/minCurl/qstringtokenizer.h \
        $$PWD/minCurl/urlgetcontent.h

    SOURCES += \
        $$PWD/minCurl/curlpp.cpp \
        $$PWD/minCurl/errorlog.cpp \
        $$PWD/minCurl/mailfetcher.cpp \
        $$PWD/minCurl/mincurl.cpp \
        $$PWD/minCurl/urlgetcontent.cpp

}

defined(withClickHouse,var){
    HEADERS += \
        $$PWD/minMysql/ClickHouseException.h \
        $$PWD/minMysql/clickhouse.h

    SOURCES += \
        $$PWD/minMysql/clickhouse.cpp
}

DISTFILES += \
        $$PWD/JSON/LICENSE \
        $$PWD/JSON/README.md \
        $$PWD/SpaceShipOP/LICENSE.md \
        $$PWD/SpaceShipOP/README.md \
        $$PWD/SpaceShipOP/qspaceship.pri \
        $$PWD/minCurl/README.md \
        $$PWD/minMysql/README.md \
        $$PWD/minMysql/minMysql.pri

HEADERS += \
    $$PWD/BoostJson/override/value_to_108300.hpp_FIX_ME \
    $$PWD/BoostJson/override/value_to_108400.hpp \
    $$PWD/BoostJson/override/value_to_108500.hpp \
    $$PWD/BoostJson/extra.h \
    $$PWD/BoostJson/fwd.h \
    $$PWD/BoostJson/depleter.h \
    #$$PWD/BoostJson/SwapperSpec.h \
    $$PWD/BoostJson/intrusivedebug.h \
    $$PWD/BoostJson/isjsonasubset.h \
    $$PWD/BoostJson/to_string.h \
    $$PWD/BoostJson/util.h \
    $$PWD/BoostJson/math.h \
    $$PWD/QStacker/CxaLevel.h \
    $$PWD/dateTime/util.h \
    $$PWD/fmtExtra/dynamic.h \
    $$PWD/fmtExtra/fromEnum.h \
    $$PWD/hash/salt.h \
    $$PWD/hash/sha.h \
    #$$PWD/isIterable.h \
    $$PWD/locale/codes.h \
    $$PWD/minMysql/mymaria.h \
    $$PWD/minMysql/DBConf.h \
    $$PWD/minMysql/checkschema.h \
    $$PWD/minMysql/runnable.h \
    $$PWD/minMysql/sqlRow.h \
    $$PWD/minMysql/sqlcomposer.h \
    $$PWD/minMysql/sqlresult.h \
    $$PWD/minMysql/sqlrowv2swap.h \
    $$PWD/misc/checkoptionalareset.h \
    $$PWD/misc/ParallelForeach.h \
    $$PWD/misc/echo.h \
    $$PWD/misc/intTypes.h \
    $$PWD/misc/swapType.h \
    $$PWD/misc/typeinfo.h \
    $$PWD/number/sanitize.h \
    $$PWD/rand/clampednormaldistribution.h \
    $$PWD/JSON/JSONReaderConst.h \
    $$PWD/JSON/jsonreader.h \
    $$PWD/SpaceShipOP/qdateship.h \
    $$PWD/SpaceShipOP/qstringship.h \
    $$PWD/caching/apcu2.h \
    $$PWD/dateTime/qdatetimev2.h \
    $$PWD/dateTime/timespecV2.h \
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
    $$PWD/misc/QCommandLineParserV2.h \
    $$PWD/misc/QDebugConfig.h \
    $$PWD/misc/QDebugHandler.h \
    $$PWD/misc/b64.h \
    $$PWD/misc/snowflake.h \
    $$PWD/misc/sourcelocation.h \
    $$PWD/mixin/CopyAssignable.h \
    $$PWD/mixin/NoCopy.h \
    $$PWD/rand/randutil.h \
    $$PWD/serialization/QDataStreamer.h \
    $$PWD/serialization/asstring.h \
    $$PWD/serialization/serialize.h \
    $$PWD/string/qstring.h \
    $$PWD/string/util.h \
    $$PWD/thread/threadstatush.h \
    $$PWD/thread/threadvector.h \
    $$PWD/thread/tmonitoring.h \
    $$PWD/versioncheck.h

SOURCES += \
    $$PWD/BoostJson/depleter.cpp \
    $$PWD/BoostJson/extra.cpp \
    $$PWD/BoostJson/intrusivedebug.cpp \
    $$PWD/BoostJson/isjsonasubset.cpp \
    $$PWD/BoostJson/to_string.cpp \
    $$PWD/BoostJson/util.cpp \
    $$PWD/BoostJson/taginvoke.cpp \
    $$PWD/BoostJson/math.cpp \
    $$PWD/HTTP/Payload.cpp \
    $$PWD/asanOption.cpp \
    $$PWD/dateTime/util.cpp \
    $$PWD/hash/salt.cpp \
    $$PWD/hash/sha.cpp \
    $$PWD/locale/codes.cpp \
    $$PWD/minMysql/checkschema.cpp \
    $$PWD/minMysql/runnable.cpp \
    $$PWD/minMysql/sqlcomposer.cpp \
    $$PWD/minMysql/sqlresult.cpp \
    $$PWD/minMysql/sqlrowv2swap.cpp \
    $$PWD/misc/checkoptionalareset.cpp \
    $$PWD/misc/echo.cpp \
    $$PWD/misc/typeinfo.cpp \
    $$PWD/number/sanitize.cpp \
    $$PWD/rand/clampednormaldistribution.cpp \
    $$PWD/JSON/jsonreader.cpp \
    $$PWD/SpaceShipOP/implementation.cpp \
    $$PWD/caching/apcu2.cpp \
    $$PWD/caching/apcuTest.cpp \
    $$PWD/filesystem/filefunction.cpp \
    $$PWD/filesystem/folder.cpp \
    $$PWD/fmtExtra/customformatter.cpp \
    $$PWD/hash/crc.cpp \
    $$PWD/locale/localev2.cpp \
    $$PWD/misc/QCommandLineParserV2.cpp \
    $$PWD/misc/QDebugHandler.cpp \
    $$PWD/misc/b64.cpp \
    $$PWD/misc/snowflake.cpp \
    $$PWD/misc/sourcelocation.cpp \
    $$PWD/rand/randutil.cpp \
    $$PWD/serialization/QDataStreamer.cpp \
    $$PWD/string/qstring.cpp \
    $$PWD/string/util.cpp \
    $$PWD/thread/threadstatush.cpp \
    $$PWD/thread/threadvector.cpp \
    $$PWD/thread/tmonitoring.cpp \
    $$PWD/gitTrick/buffer.cpp \
    $$PWD/versioncheck.cpp

#this will force it to recompile each time, to have fresh data
linux {
    QMAKE_PRE_LINK += touch $$PWD/gitTrick/buffer.cpp
}
#ofc win is laking the most basic command, so we skip for now



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
        $$PWD/mapExtensor/NotFoundMixin.h \
        $$PWD/mapExtensor/joinVector.h

DISTFILES += /
        $$PWD/mapExtensor/README.md

SOURCES += \
    $$PWD/mapExtensor/indexedvector.cpp \
        $$PWD/mapExtensor/lockguardv2.cpp \
    $$PWD/mapExtensor/mapV2.cpp \
    $$PWD/mapExtensor/rwguard.cpp
    
    
#for rapidJson
#QMAKE_CXXFLAGS += -msse4.2


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
    $$PWD/minMysql/MITLS.h \
    $$PWD/minMysql/min_mysql.h  \
    $$PWD/minMysql/ttlcache.h \
    $$PWD/minMysql/utilityfunctions.h
    
SOURCES += \
    $$PWD/minMysql/MITLS.cpp \
    $$PWD/minMysql/min_mysql.cpp \
    $$PWD/minMysql/ttlcache.cpp \
    $$PWD/minMysql/utilityfunctions.cpp
    
DISTFILES += /
        $$PWD/README.md
	
DISTFILES += \
        $$PWD/README.md

#is you perform QT += network somewhere
defined(HAS_QT_NETWORK, var) {
        HEADERS += $$PWD/qhostaddress.h
        SOURCES += $$PWD/qhostaddress.cpp
}




#Mustache
INCLUDEPATH += $$PWD/mustache/
SOURCES += \
    $$PWD/mustache/renderer.cpp \
    $$PWD/mustache/extra.cpp

HEADERS += $$PWD/mustache/boost/mustache.hpp \
    $$PWD/mustache/extra.h

#Stuff no longer used
    #$$PWD/misc/UaDecoder.h \
    #$$PWD/misc/UaDecoder.cpp \
    #$$PWD/misc/slacksender.cpp \
    #$$PWD/misc/slacksender.h \
    #$$PWD/misc/twilio.h \
    #$$PWD/misc/twilio.cpp \

