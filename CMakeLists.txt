if (WIN32)
else()
    set(CMAKE_CXX_COMPILER "/usr/bin/g++-6")
endif()

cmake_minimum_required(VERSION 2.6)
project(LALRT)
cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0008 OLD)

# Boost and Poco paths:
if (MSVC)
    include(local_paths.cmake)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

#
# Boost Setup
#
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
if (WIN32)
    add_definitions(-DBOOST_USE_WINAPI_VERSION=0x0501)
    if (MSVC)
        # disable autolinking in boost and some other warnings
        add_definitions(-DBOOST_ALL_NO_LIB -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0501)
    endif()
    set(Boost_THREADAPI win32)
else()
    add_definitions(-DBOOST_ALL_NO_LIB)
endif()
if (WIN32)
    find_package(Boost 1.60 COMPONENTS
        log_setup
        log
        system
        filesystem
        locale
        thread
        date_time
        regex
        unit_test_framework
    REQUIRED)
else()
    find_package(Boost 1.50 COMPONENTS
        log_setup
        log
        system
        filesystem
        locale
        thread
        date_time
        regex
        unit_test_framework
    REQUIRED)
endif()

#
# POCO Setup
#
if (WIN32)
    if (!MSVC)
        set(POCO_LIB     /mingw32/lib/)
        set(POCO_INCLUDE /mingw32/include/Poco/)
    endif()
endif()

set(POCO_INCLUDEDIRS
    ${POCO_INCLUDE}/JSON/include
    ${POCO_INCLUDE}/Foundation/include
    ${POCO_INCLUDE}/Util/include
    ${POCO_INCLUDE}/Net/include
    ${POCO_INCLUDE}/NetSSL_OpenSSL/include
    ${POCO_INCLUDE}/Crypto/include
    ${POCO_INCLUDE}/../OpenSSL-Win32/include
)

set(POCO_LIBRARYDIRS ${POCO_LIB})
if (WIN32)
    if (MSVC)
        find_library(POCO_LIBRARY           NAMES PocoFoundationmd  PATHS ${POCO_LIBRARYDIRS})
        find_library(POCO_NET_LIBRARY       NAMES PocoNetmd         HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_UTIL_LIBRARY      NAMES PocoUtilmd        HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_JSON_LIBRARY      NAMES PocoJSONmd        HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_NETSSL_LIBRARY    NAMES PocoNetSSLmd      HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_XML_LIBRARY       NAMES PocoXMLmd         HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_CRYPTO_LIBRARY    NAMES PocoCryptomd      HINTS ${POCO_LIBRARYDIRS})
    else()
        find_library(POCO_LIBRARY           NAMES PocoFoundation    PATHS ${POCO_LIBRARYDIRS})
        find_library(POCO_NET_LIBRARY       NAMES PocoNet           HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_UTIL_LIBRARY      NAMES PocoUtil          HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_JSON_LIBRARY      NAMES PocoJSON          HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_NETSSL_LIBRARY    NAMES PocoNetSSL        HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_XML_LIBRARY       NAMES PocoXML           HINTS ${POCO_LIBRARYDIRS})
        find_library(POCO_CRYPTO_LIBRARY    NAMES PocoCrypto        HINTS ${POCO_LIBRARYDIRS})
    endif()
else()
    find_library(POCO_LIBRARY           NAMES PocoFoundation PATHS ${POCO_LIBRARYDIRS})
    find_library(POCO_NET_LIBRARY       NAMES PocoNet        HINTS ${POCO_LIBRARYDIRS})
    find_library(POCO_UTIL_LIBRARY      NAMES PocoUtil       HINTS ${POCO_LIBRARYDIRS})
    find_library(POCO_NETSSL_LIBRARY    NAMES PocoNetSSL     HINTS ${POCO_LIBRARYDIRS})
    find_library(POCO_XML_LIBRARY       NAMES PocoXML        HINTS ${POCO_LIBRARYDIRS})
    find_library(POCO_CRYPTO_LIBRARY    NAMES PocoCrypto     HINTS ${POCO_LIBRARYDIRS})
    set(POCO_JSON_LIBRARY)
endif()

set(POCO_LIBRARIES
    ${POCO_LIBRARY}
    ${POCO_NET_LIBRARY}
    ${POCO_NETSSL_LIBRARY}
    ${POCO_JSON_LIBRARY}
    ${POCO_XML_LIBRARY}
    ${POCO_CRYPTO_LIBRARY}
    ${POCO_UTIL_LIBRARY})

if (WIN32)
    add_definitions(-DPOCO_WIN32_UTF8 -DPOCO_STATIC)
else()
    add_definitions(-DPOCO_STATIC)
endif()

if (WIN32)
    if (MSYS)
        find_library(ICONV_LIBRARY  NAMES libiconv.a ) # iconv)
        find_library(ICUIN_LIBRARY  NAMES libsicuin.a) # icuin)
        find_library(ICUUC_LIBRARY  NAMES libsicuuc.a) # icuuc)
        find_library(ICUDT_LIBRARY  NAMES libsicudt.a) # icuuc)
        set(BOOST_SUPPORT_LIBS Boost::disable_autolinking
            ${ICONV_LIBRARY} ${ICUIN_LIBRARY} ${ICUUC_LIBRARY} ${ICUDT_LIBRARY})
    else()
        set(BOOST_SUPPORT_LIBS Boost::disable_autolinking)
    endif()
else()
    find_library(ICUIN_LIBRARY  NAMES libicui18n.a) # icuin)
    find_library(ICUUC_LIBRARY  NAMES libicuuc.a)   # icuuc)
    find_library(ICUDT_LIBRARY  NAMES libicudata.a) # icuuc)
    set(BOOST_SUPPORT_LIBS ${ICUIN_LIBRARY} ${ICUUC_LIBRARY} ${ICUDT_LIBRARY})
endif()

#
# Project Definition: lalrt_support static library
#
add_library(lalrt_support STATIC
    external/sqlite3/sqlite3.c

    lib/base/vval.cpp
    lib/base/vval_util.cpp
    lib/base/endian.cpp
    lib/base/JSON.cpp
    lib/base/crc.cpp
    lib/base/csv.cpp
    lib/base/sqldb.cpp
    lib/base/http.cpp
    lib/base/util.cpp

    lib/lua/lua_instance.cpp

    lib/rt/process.cpp
    lib/rt/syslib.cpp
    lib/rt/sqldblib.cpp
    lib/rt/utillib.cpp
    lib/rt/httplib.cpp
    lib/rt/log.cpp
    lib/rt/lua_thread.cpp
)

#
# Project Definition: lalrt_lua static library
#
add_library(lalrt_lua STATIC
    lua/src/lapi.cpp
    lua/src/lauxlib.cpp
    lua/src/lbaselib.cpp
    lua/src/lbitlib.cpp
    lua/src/lcode.cpp
    lua/src/lcorolib.cpp
    lua/src/lctype.cpp
    lua/src/ldblib.cpp
    lua/src/ldebug.cpp
    lua/src/ldo.cpp
    lua/src/ldump.cpp
    lua/src/lfunc.cpp
    lua/src/lgc.cpp
    lua/src/linit.cpp
    lua/src/liolib.cpp
    lua/src/llex.cpp
    lua/src/lmathlib.cpp
    lua/src/lmem.cpp
    lua/src/loadlib.cpp
    lua/src/lobject.cpp
    lua/src/lopcodes.cpp
    lua/src/loslib.cpp
    lua/src/lparser.cpp
    lua/src/lstate.cpp
    lua/src/lstring.cpp
    lua/src/lstrlib.cpp
    lua/src/ltable.cpp
    lua/src/ltablib.cpp
    lua/src/ltm.cpp
    lua/src/lundump.cpp
    lua/src/lutf8lib.cpp
    lua/src/lvm.cpp
    lua/src/lzio.cpp
)

#
# Qt Setup
#
find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5PrintSupport)
find_package(Qt5Gui)

if (Qt5Widgets_FOUND)
    add_definitions(-DHAS_QT5=1)
    if (NOT WIN32)
        add_definitions(-fPIC)
    endif()
    add_library(lalrt_qt STATIC
        modules/qt/init.cpp
        modules/qt/line_editor.cpp
    )
    if (MSVC)
        set(QT_LIBRARIES Qt5::Core Qt5::Widgets)
    else()
        set(QT_LIBRARIES Qt5::Core Qt5::Widgets) # msys2: -ljasper -ldbus-1 -lwebp)
    endif()
    set(QT_INCLUDEDIRS ${Qt5Widgets_INCLUDE_DIRS}
                       ${Qt5Core_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(lalrt_support lalrt_qt ${QT_LIBRARIES})
else()
    add_definitions(-DHAS_QT5=0)
    set(QT_LIBRARIES)
    set(QT_INCLUDEDIRS)
endif()

#
# Platform specific stuff
#
TARGET_LINK_LIBRARIES(lalrt_support lalrt_lua)
if (WIN32)
    if (MSVC)
        set(PLATFORM_LIBS_ADD) # -lws2_32)
    else()
        set(PLATFORM_LIBS_ADD -lws2_32)
    endif()
else()
    set(PLATFORM_LIBS_ADD -ldl)
endif()
TARGET_LINK_LIBRARIES(lalrt_support ${PLATFORM_LIBS_ADD})

#
# Project Definition: lalrt
#
# TODO: statically link libgcc and libstdc++
add_executable(LALRT main.cpp)
if (WIN32)
    # Suppress a warning on windows 7 with VS2015 in sqlite3.c by declaring
    # GetVersionEx as deprecated. The preprocessor definitions don't seem to be
    # accurate enough.
    add_definitions(-DSQLITE_WIN32_GETVERSIONEX=0)
endif()
if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-placement-new")
endif()
#add_definitions(-Wno-placement-new)
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
include_directories(${Boost_INCLUDE_DIRS} ${POCO_INCLUDEDIRS} ${QT_INCLUDEDIRS} lib/ external/)
target_link_libraries(LALRT lalrt_support ${Boost_LIBRARIES} ${BOOST_SUPPORT_LIBS} ${POCO_LIBRARIES} )

set_target_properties(LALRT PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}
)

#
# Sub-Project Definition: unit tests
#
add_subdirectory(tests)
enable_testing()
add_test(NAME vv_test     COMMAND VVTest)
add_test(NAME msging_test COMMAND VVTestMsging)
add_test(NAME lua_test    COMMAND VVTestLua)
add_test(NAME rt_test     COMMAND VVTestRT)
add_test(NAME sqldb_test  COMMAND VVTestSQLDB)
