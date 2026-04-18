cmake_minimum_required(VERSION 4.2)
project(rbk LANGUAGES CXX)

# Out-of-source builds only. Standalone workflow uses build/ next to this repo (CMakePresets.json default).
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-source configures are not supported (they pollute the source tree).\n"
        "Put generated files under the build directory — convention is ./build next to the repo:\n"
        "  cmake --preset default\n"
        "  cmake --build build\n"
        "Or equivalently:\n"
        "  cmake -B build\n"
        "  cmake --build build"
    )
endif()

# ── Options ──────────────────────────────────────────────────────────────────
option(RBK_WITH_BOOST_BEAST  "Boost.Beast HTTP server + router"       OFF)
option(RBK_WITH_BOOST_MYSQL  "Boost.MySQL (separate compilation)"     OFF)
option(RBK_WITH_ASAN         "AddressSanitizer"                       OFF)
option(RBK_WITH_JEMALLOC     "jemalloc allocator"                     OFF)
option(RBK_WITH_SODIUM       "libsodium crypto"                       OFF)
option(RBK_WITH_SSL          "OpenSSL + TOTP"                         OFF)
option(RBK_WITH_REPROC       "reproc process launcher"                OFF)
option(RBK_WITH_ZIPPER       "libzip"                                 OFF)
option(RBK_WITH_MAXMIND      "MaxMind GeoIP"                         OFF)
option(RBK_WITH_MINCURL      "libcurl wrapper"                        OFF)
option(RBK_WITH_CLICKHOUSE   "ClickHouse client"                      OFF)
option(RBK_WITH_QRCODE       "QR code generation"                     OFF)
option(RBK_WITH_APCU         "APCu-style in-memory caching"           OFF)
option(RBK_WITH_QT_NETWORK   "Qt Network extras (SpaceShipOP)"        OFF)
option(RBK_WITH_GIT_TRICK    "Re-embed git info on every build"       OFF)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(RBK_BUILD_TESTS "Build rbk unit tests (Boost.Test)" ON)
else()
    option(RBK_BUILD_TESTS "Build rbk unit tests (Boost.Test)" OFF)
endif()

# ── Mutual exclusion ────────────────────────────────────────────────────────
if(RBK_WITH_ASAN AND RBK_WITH_JEMALLOC)
    message(FATAL_ERROR "RBK_WITH_ASAN and RBK_WITH_JEMALLOC cannot be used together -- ASan replaces malloc")
endif()

# ── Qt location ─────────────────────────────────────────────────────────────
# Prefer CMAKE_PREFIX_PATH on your top-level project configure (same Qt for the
# main app and add_subdirectory(rbk)). When configuring rbk standalone, default
# RBK_QT_PREFIX matches a typical Linux Qt installer kit (bin/qmake under prefix).
set(RBK_QT_PREFIX_DEFAULT "")
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(RBK_QT_PREFIX_DEFAULT "/opt/Qt/6.8.3/gcc_64")
endif()
set(RBK_QT_PREFIX "${RBK_QT_PREFIX_DEFAULT}" CACHE PATH
    "Prepend to CMAKE_PREFIX_PATH for find_package(Qt6). Empty: do not prepend. Default when configuring rbk alone: /opt/Qt/6.8.3/gcc_64."
)
unset(RBK_QT_PREFIX_DEFAULT)
if(RBK_QT_PREFIX)
    list(PREPEND CMAKE_PREFIX_PATH "${RBK_QT_PREFIX}")
endif()

# ── Find dependencies ───────────────────────────────────────────────────────
find_package(Qt6 QUIET COMPONENTS Core)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "Qt6 Core was not found.\n"
        "  Set CMAKE_PREFIX_PATH to your Qt kit (the directory that contains lib/cmake/Qt6/;\n"
        "  qmake is at <prefix>/bin/qmake). Use the same prefix for your main project;\n"
        "  add_subdirectory(rbk) inherits it.\n"
        "  Example:\n"
        "      cmake -B build -S . -DCMAKE_PREFIX_PATH=/opt/Qt/6.8.3/gcc_64\n"
        "  Or set -DRBK_QT_PREFIX=... (default for standalone configure: /opt/Qt/6.8.3/gcc_64).\n"
        "  If a system Qt6 is installed and discoverable, use -DRBK_QT_PREFIX= or clear the cache entry."
    )
endif()
if(RBK_BUILD_TESTS)
    find_package(Boost 1.83 REQUIRED COMPONENTS unit_test_framework)
else()
    find_package(Boost 1.83 REQUIRED)
endif()
find_package(fmt REQUIRED)

include(FindPkgConfig)
pkg_check_modules(MARIADB REQUIRED IMPORTED_TARGET libmariadb)

if(RBK_WITH_SODIUM)
    pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)
endif()

if(RBK_WITH_SSL)
    find_package(OpenSSL REQUIRED)
endif()

if(RBK_WITH_MINCURL)
    find_package(CURL REQUIRED)
endif()

if(RBK_WITH_MAXMIND)
    pkg_check_modules(MAXMINDDB REQUIRED IMPORTED_TARGET libmaxminddb)
endif()

if(RBK_WITH_ZIPPER)
    pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip)
endif()

if(RBK_WITH_QT_NETWORK)
    find_package(Qt6 REQUIRED COMPONENTS Network)
endif()

if(RBK_WITH_REPROC)
    find_package(reproc++ REQUIRED)
endif()

if(RBK_WITH_JEMALLOC)
    pkg_check_modules(JEMALLOC REQUIRED IMPORTED_TARGET jemalloc)
endif()

# ── Core sources (always built) ─────────────────────────────────────────────
set(RBK_SOURCES
    asanOption.cpp
    versioncheck.cpp

    # BoostJson
    BoostJson/depleter.cpp
    BoostJson/extra.cpp
    BoostJson/intrusivedebug.cpp
    BoostJson/isjsonasubset.cpp
    BoostJson/math.cpp
    BoostJson/taginvoke.cpp
    BoostJson/to_string.cpp
    BoostJson/util.cpp

    # dateTime
    dateTime/util.cpp

    # filesystem
    filesystem/filefunction.cpp
    filesystem/folder.cpp
    filesystem/suffix.cpp

    # fmtExtra
    fmtExtra/customformatter.cpp

    # gitTrick
    gitTrick/buffer.cpp

    # hash
    hash/crc.cpp
    hash/salt.cpp
    hash/sha.cpp
    hash/string.cpp

    # HTTP
    HTTP/mime.cpp
    HTTP/Payload.cpp
    HTTP/PMFCGI.cpp
    HTTP/url.cpp
    HTTP/util.cpp

    # JSON
    JSON/jsonreader.cpp
    JSON/safeGet.cpp
    JSON/various.cpp

    # locale
    locale/codes.cpp
    locale/localev2.cpp

    # log
    log/log.cpp

    # mapExtensor
    mapExtensor/ankerv2.cpp
    mapExtensor/indexedvector.cpp
    mapExtensor/lockguardv2.cpp
    mapExtensor/mapV2.cpp
    mapExtensor/missingkeyex.cpp
    mapExtensor/rwguard.cpp

    # minMysql
    minMysql/checkschema.cpp
    minMysql/min_mysql.cpp
    minMysql/MITLS.cpp
    minMysql/rowswap.cpp
    minMysql/runnable.cpp
    minMysql/sqlbuffering.cpp
    minMysql/sqlcomposer.cpp
    minMysql/sqlresult.cpp
    minMysql/sqlrowv2.cpp
    minMysql/sqlrowv2swap.cpp
    minMysql/ttlcache.cpp
    minMysql/utilityfunctions.cpp

    # misc
    misc/b64.cpp
    misc/base32.cpp
    misc/checkoptionalareset.cpp
    misc/csv.cpp
    misc/echo.cpp
    misc/QCommandLineParserV2.cpp
    misc/QDebugHandler.cpp
    misc/QElapsedTimerV2.cpp
    misc/sleep.cpp
    misc/snowflake.cpp
    misc/sourcelocation.cpp
    misc/typeinfo.cpp

    # mustache
    mustache/extra.cpp
    mustache/renderer.cpp

    # number
    number/doubleoperator.cpp
    number/sanitize.cpp

    # QStacker
    QStacker/exceptionv2.cpp
    QStacker/httpexception.cpp
    QStacker/qstacker.cpp

    # rand
    rand/clampednormaldistribution.cpp
    rand/randutil.cpp

    # rapidjson
    rapidjson/rapidAssert.cpp

    # serialization
    serialization/QDataStreamer.cpp

    # SpaceShipOP
    SpaceShipOP/implementation.cpp

    # string
    string/comparator.cpp
    string/qstring.cpp
    string/qstringview.cpp
    string/stringoso.cpp
    string/util.cpp

    # thread
    thread/threadstatush.cpp
    thread/threadvector.cpp
    thread/tmonitoring.cpp
)

set(RBK_HEADERS
    versioncheck.h

    # BoostJson
    BoostJson/array.h
    BoostJson/depleter.h
    BoostJson/extra.h
    BoostJson/fwd.h
    BoostJson/intrusivedebug.h
    BoostJson/isjsonasubset.h
    BoostJson/JsonRes.h
    BoostJson/math.h
    BoostJson/tagInvokeCrono.h
    BoostJson/taginvoke.h
    BoostJson/to_string.h
    BoostJson/util.h
    BoostJson/override/value_to_108400.hpp
    BoostJson/override/value_to_108500.hpp

    # concept
    concept/concepts.h
    concept/isSharedPtr.h

    # dateTime
    dateTime/qdatetimev2.h
    dateTime/timespecV2.h
    dateTime/util.h

    # defines
    defines/stringDefine.h

    # filesystem
    filesystem/ffCommon.h
    filesystem/filefunction.h
    filesystem/folder.h
    filesystem/suffix.h

    # fmtExtra
    fmtExtra/customformatter.h
    fmtExtra/dynamic.h
    fmtExtra/fromEnum.h
    fmtExtra/includeMe.h

    # gitTrick
    gitTrick/buffer.h

    # hash
    hash/crc.h
    hash/rapidhash.h
    hash/salt.h
    hash/sha.h
    hash/string.h

    # HTTP
    HTTP/mime.h
    HTTP/PMFCGI.h
    HTTP/url.h
    HTTP/util.h

    # JSON
    JSON/JSONReaderConst.h
    JSON/jsonreader.h
    JSON/safeGet.h
    JSON/various.h

    # locale
    locale/codes.h
    locale/localev2.h

    # log
    log/log.h

    # magicEnum
    magicEnum/magic_enum.hpp
    magicEnum/magic_from_string.hpp

    # mapExtensor
    mapExtensor/ankerl_unordered_dense.h
    mapExtensor/ankerv2.h
    mapExtensor/fixedSizeVector.h
    mapExtensor/hmap.h
    mapExtensor/indexedvector.h
    mapExtensor/joinVector.h
    mapExtensor/lockguardv2.h
    mapExtensor/mapV2.h
    mapExtensor/missingkeyex.h
    mapExtensor/NotFoundMixin.h
    mapExtensor/OptionalV2.h
    mapExtensor/qmapV2.h
    mapExtensor/RRList.h
    mapExtensor/rwguard.h
    mapExtensor/ThreadSafeMultiIndex.hpp
    mapExtensor/valueMitWarning.h
    mapExtensor/vectorV2.h

    # minMysql
    minMysql/checkschema.h
    minMysql/checkSchema/CheckSchemaConf.h
    minMysql/checkSchema/CheckSchemaDescribe.h
    minMysql/DBConf.h
    minMysql/min_mysql.h
    minMysql/MITLS.h
    minMysql/mymaria.h
    minMysql/rowswap.h
    minMysql/runnable.h
    minMysql/sqlbuffering.h
    minMysql/sqlcomposer.h
    minMysql/sqlresult.h
    minMysql/sqlRow.h
    minMysql/sqlrowv2.h
    minMysql/sqlrowv2swap.h
    minMysql/ttlcache.h
    minMysql/utilityfunctions.h

    # misc
    misc/b64.h
    misc/base32.h
    misc/checkoptionalareset.h
    misc/controlFlowMacro.h
    misc/csv.h
    misc/decimate.h
    misc/echo.h
    misc/intTypes.h
    misc/NanoSpammerConfigDescribe.h
    misc/ParallelForeach.h
    misc/QCommandLineParserV2.h
    misc/QDebugConfig.h
    misc/QDebugHandler.h
    misc/QElapsedTimerV2.h
    misc/sleep.h
    misc/snowflake.h
    misc/sourcelocation.h
    misc/swapType.h
    misc/typeinfo.h

    # mixin
    mixin/CopyAssignable.h
    mixin/NoCopy.h

    # mustache
    mustache/boost/mustache.hpp
    mustache/extra.h

    # number
    number/converter.h
    number/doubleoperator.h
    number/intTypes.h
    number/sanitize.h

    # QStacker
    QStacker/backward.hpp
    QStacker/CxaLevel.h
    QStacker/exceptionv2.h
    QStacker/httpexception.h
    QStacker/qstacker.h

    # RAII
    RAII/resetAfterUse.h

    # rand
    rand/clampednormaldistribution.h
    rand/randutil.h

    # rapidjson
    rapidjson/includeMe.h

    # serialization
    serialization/asstring.h
    serialization/QDataStreamer.h
    serialization/serialize.h

    # SpaceShipOP
    SpaceShipOP/qdateship.h
    SpaceShipOP/qstringship.h

    # string
    string/comparator.h
    string/concept.h
    string/qstring.h
    string/qstringview.h
    string/stringoso.h
    string/util.h

    # thread
    thread/threadstatush.h
    thread/threadvector.h
    thread/tmonitoring.h

    # types
    types/isOptional.h
)

# ── Optional: Boost.Beast ────────────────────────────────────────────────────
if(RBK_WITH_BOOST_BEAST)
    list(APPEND RBK_SOURCES
        HTTP/beast.cpp
        HTTP/beastConfig.cpp
        HTTP/router.cpp
        HTTP/select2.cpp
    )
    list(APPEND RBK_HEADERS
        HTTP/beast.h
        HTTP/beastConfig.h
        HTTP/Payload.h
        HTTP/router.h
        HTTP/select2.h
    )
endif()

# ── Optional: Boost.MySQL ───────────────────────────────────────────────────
if(RBK_WITH_BOOST_MYSQL)
    list(APPEND RBK_SOURCES BoostMysql/includeme.cpp)
    list(APPEND RBK_HEADERS BoostMysql/includeme.h)
endif()

# ── Optional: jemalloc ──────────────────────────────────────────────────────
if(RBK_WITH_JEMALLOC)
    list(APPEND RBK_SOURCES jemalloc/jemutil.cpp)
    list(APPEND RBK_HEADERS jemalloc/jemutil.h)
endif()

# ── Optional: libsodium ─────────────────────────────────────────────────────
if(RBK_WITH_SODIUM)
    list(APPEND RBK_SOURCES Sodium/crypto.cpp)
    list(APPEND RBK_HEADERS Sodium/crypto.h)
endif()

# ── Optional: OpenSSL + TOTP ────────────────────────────────────────────────
if(RBK_WITH_SSL)
    list(APPEND RBK_SOURCES totp/totp.cpp)
    list(APPEND RBK_HEADERS totp/totp.h)
endif()

# ── Optional: reproc ────────────────────────────────────────────────────────
if(RBK_WITH_REPROC)
    list(APPEND RBK_SOURCES misc/executor.cpp)
    list(APPEND RBK_HEADERS misc/executor.h)
endif()

# ── Optional: libzip ────────────────────────────────────────────────────────
if(RBK_WITH_ZIPPER)
    list(APPEND RBK_SOURCES misc/zip.cpp)
    list(APPEND RBK_HEADERS misc/zip.h)
endif()

# ── Optional: libcurl ───────────────────────────────────────────────────────
if(RBK_WITH_MINCURL)
    list(APPEND RBK_SOURCES
        minCurl/curlpp.cpp
        minCurl/errorlog.cpp
        minCurl/mailfetcher.cpp
        minCurl/mincurl.cpp
    )
    list(APPEND RBK_HEADERS
        minCurl/curlpp.h
        minCurl/errorlog.h
        minCurl/mailfetcher.h
        minCurl/mincurl.h
        minCurl/qstringtokenizer.h
    )
endif()

# ── Optional: ClickHouse ────────────────────────────────────────────────────
if(RBK_WITH_CLICKHOUSE)
    list(APPEND RBK_SOURCES minMysql/clickhouse.cpp)
    list(APPEND RBK_HEADERS
        minMysql/clickhouse.h
        minMysql/ClickHouseException.h
    )
endif()

# ── Optional: QR code ───────────────────────────────────────────────────────
if(RBK_WITH_QRCODE)
    list(APPEND RBK_SOURCES
        QR/qr_code.cpp
        QR/qrcodegen.cpp
    )
    list(APPEND RBK_HEADERS
        QR/qr_code.h
        QR/qrcodegen.hpp
    )
endif()

# ── Optional: APCU caching ──────────────────────────────────────────────────
if(RBK_WITH_APCU)
    list(APPEND RBK_SOURCES
        caching/apcu2.cpp
        caching/apcuTest.cpp
        caching/cachable.cpp
    )
    list(APPEND RBK_HEADERS
        caching/apcu2.h
        caching/cachable.h
    )
endif()

# ── Optional: Qt Network extras ─────────────────────────────────────────────
if(RBK_WITH_QT_NETWORK)
    list(APPEND RBK_SOURCES qhostaddress.cpp)
    list(APPEND RBK_HEADERS qhostaddress.h)
endif()

# ── Git version embedding ───────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gitTrick/submoduleInfo")
    # use existing file
else()
    file(TOUCH "${CMAKE_CURRENT_SOURCE_DIR}/gitTrick/submoduleInfo")
endif()

# Qt resource for submodule info
qt6_add_resources(RBK_QRC gitTrick/resources.qrc)
list(APPEND RBK_SOURCES ${RBK_QRC})

# ── Library target ──────────────────────────────────────────────────────────
add_library(rbk STATIC ${RBK_SOURCES} ${RBK_HEADERS})
add_library(rbk::rbk ALIAS rbk)

# ── C++26 with GNU extensions (propagates to dependents via PUBLIC feature requirement)
set_target_properties(rbk PROPERTIES CXX_EXTENSIONS ON)
target_compile_features(rbk PUBLIC cxx_std_26)

# ── Compiler warnings & hardening (private -- don't leak to consumers) ──────
target_compile_options(rbk PRIVATE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion
    -Wunused -Wunused-function
    -fno-permissive
    -Werror=return-type
    -fvisibility=hidden
    -ftrivial-auto-var-init=pattern
    -fdiagnostics-all-candidates
)

# ── Include directories ─────────────────────────────────────────────────────
target_include_directories(rbk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson
        ${CMAKE_CURRENT_SOURCE_DIR}/mustache
)

# Treat Qt headers as system headers to suppress warnings
get_target_property(_qt_inc Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
if(_qt_inc)
    target_include_directories(rbk SYSTEM PRIVATE ${_qt_inc})
endif()

# ── Compile definitions ─────────────────────────────────────────────────────
target_compile_definitions(rbk
    PUBLIC
        QT_DEPRECATED_WARNINGS
    PRIVATE
        BuildDir="${CMAKE_BINARY_DIR}"
        BasePath="${CMAKE_SOURCE_DIR}"
)

# ── Git describe ─────────────────────────────────────────────────────────────
if(UNIX)
    execute_process(
        COMMAND git -C "${CMAKE_SOURCE_DIR}" describe --always --dirty --abbrev=99
        OUTPUT_VARIABLE _git_status
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_git_status)
        target_compile_definitions(rbk PRIVATE GIT_STATUS="${_git_status}")
    endif()

    if(RBK_WITH_GIT_TRICK)
        add_custom_command(
            TARGET rbk PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_SOURCE_DIR}/gitTrick/buffer.cpp"
            COMMAND git -C "${CMAKE_SOURCE_DIR}" submodule foreach
                    git describe --always --abbrev=99 --dirty
                    > "${CMAKE_SOURCE_DIR}/rbk/gitTrick/submoduleInfo"
            COMMENT "Refreshing git submodule info"
        )
    endif()
endif()

# ── Embed source/build dirs (GCC --embed-dir) ───────────────────────────────
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(rbk PRIVATE
        --embed-dir=${CMAKE_BINARY_DIR}
        --embed-dir=${CMAKE_SOURCE_DIR}
    )
endif()

# ── Core link dependencies ──────────────────────────────────────────────────
target_link_libraries(rbk
    PUBLIC
        Qt6::Core
        Boost::boost
        fmt::fmt
        PkgConfig::MARIADB
)

if(UNIX AND NOT APPLE)
    target_link_libraries(rbk PUBLIC dw dl)
endif()

if(WIN32)
    target_link_libraries(rbk PUBLIC dbghelp)
endif()

# ── Optional link dependencies ──────────────────────────────────────────────
if(RBK_WITH_BOOST_BEAST)
    # Beast is header-only but needs Asio / system
endif()

if(RBK_WITH_BOOST_MYSQL)
    target_compile_definitions(rbk PUBLIC BOOST_MYSQL_SEPARATE_COMPILATION)
endif()

if(RBK_WITH_JEMALLOC)
    target_compile_definitions(rbk PUBLIC WITH_Jemalloc)
    # jemalloc must be linked first to override malloc
    target_link_libraries(rbk PUBLIC
        $<LINK_GROUP:RESCAN,PkgConfig::JEMALLOC>
    )
    set_property(TARGET rbk APPEND PROPERTY LINK_LIBRARIES PkgConfig::JEMALLOC)
endif()

if(RBK_WITH_SODIUM)
    target_link_libraries(rbk PUBLIC PkgConfig::SODIUM)
endif()

if(RBK_WITH_SSL)
    target_link_libraries(rbk PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

if(RBK_WITH_REPROC)
    target_compile_definitions(rbk PUBLIC WITH_REPROC)
    target_link_libraries(rbk PUBLIC reproc++)
endif()

if(RBK_WITH_ZIPPER)
    target_link_libraries(rbk PUBLIC PkgConfig::LIBZIP)
endif()

if(RBK_WITH_MAXMIND)
    target_link_libraries(rbk PUBLIC
        PkgConfig::MAXMINDDB
        ${CMAKE_CURRENT_SOURCE_DIR}/GeoLite2PP/libgeolite2++.a
    )
    target_include_directories(rbk PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GeoLite2PP>
    )
endif()

if(RBK_WITH_MINCURL)
    target_compile_definitions(rbk PUBLIC useMinCurl)
    target_link_libraries(rbk PUBLIC CURL::libcurl)
endif()

if(RBK_WITH_QT_NETWORK)
    target_link_libraries(rbk PUBLIC Qt6::Network)
endif()

# ── AddressSanitizer ────────────────────────────────────────────────────────
if(RBK_WITH_ASAN)
    target_compile_options(rbk PUBLIC -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(rbk PUBLIC -fsanitize=address -fno-omit-frame-pointer)
endif()

# ── Unit tests (Boost.Test) ─────────────────────────────────────────────────
if(RBK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
