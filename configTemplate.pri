WITH_HTTP = false
INCLUDEPATH += $$PWD/

# Prebuilt library mode (optional — instead of include(rbk/rbk.pri)):
# debug { RBK_LIB_PREFIX = /opt/rbk/debug }
# else  { RBK_LIB_PREFIX = /opt/rbk/relwithdebinfo }
# include(rbk/rbk_link.pri)
#
# Or link from a build tree without installing:
# RBK_LIB_DIR = $$PWD/rbk/build-debug
# include(rbk/rbk_link.pri)
