# Link a prebuilt librbk.a instead of compiling RBK sources.
#
# Set RBK_LIB_PREFIX in config.pri to the install prefix (or use RBK_LIB_DIR for
# a raw build tree). Match debug vs release to your app build type.
#
# Example config.pri:
#   debug { RBK_LIB_PREFIX = /opt/rbk/debug }
#   else  { RBK_LIB_PREFIX = /opt/rbk/relwithdebinfo }
#   include(rbk/rbk_link.pri)
#
# Or point at a build directory without installing:
#   RBK_LIB_DIR = $$PWD/rbk/build-debug
#   include(rbk/rbk_link.pri)

isEmpty(RBK_LIB_PREFIX):isEmpty(RBK_LIB_DIR) {
    error("Set RBK_LIB_PREFIX (install prefix) or RBK_LIB_DIR (path to build-debug etc.) in config.pri")
}

DEFINES += QT_DEPRECATED_WARNINGS

QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]/QtCore

isEmpty(RBK_LIB_DIR) {
    INCLUDEPATH += $$RBK_LIB_PREFIX/include
    INCLUDEPATH += $$RBK_LIB_PREFIX/include/rbk
    exists($$RBK_LIB_PREFIX/lib64/librbk.a) {
        RBK_LIB_PATH = $$RBK_LIB_PREFIX/lib64/librbk.a
    } else {
        RBK_LIB_PATH = $$RBK_LIB_PREFIX/lib/librbk.a
    }
} else {
    # Build tree: e.g. RBK_LIB_DIR = $$PWD/rbk/build-debug
    INCLUDEPATH += $$RBK_LIB_DIR/../..
    INCLUDEPATH += $$RBK_LIB_DIR/..
    RBK_LIB_PATH = $$RBK_LIB_DIR/librbk.a
}

LIBS += $$RBK_LIB_PATH

linux {
    LIBS += -ldw
    LIBS += -ldl
    LIBS += -lfmt
    LIBS += -lmariadb
}
win32 {
    LIBS += -ldbghelp
}

# If your prebuilt rbk was configured with optional RBK_WITH_* / WITH_* features,
# add the same extra LIBS + DEFINES here as in rbk.pri (e.g. -lsodium, -lssl, -lcurl).
