SOURCES += $$PWD/rapidAssert.cpp 
	
HEADERS += $$PWD/includeMe.h

#looks like is now required
QMAKE_CXXFLAGS += -msse4.2
