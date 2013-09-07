#-------------------------------------------------
#
# Project created by QtCreator 2013-06-21T21:58:54
#
#-------------------------------------------------

QT       += core gui network widgets

TARGET = LoE_PrivateServer
TEMPLATE = app


SOURCES += main.cpp\
		widget.cpp \
	tcp.cpp \
	udp.cpp \
	messages.cpp \
	utils.cpp \
	pingTimeout.cpp \
	character.cpp \
	scene.cpp \
	dataType.cpp \
	sync.cpp

HEADERS  += widget.h \
	character.h \
	message.h \
	utils.h \
	scene.h \
	dataType.h \
	sync.h

FORMS    += widget.ui

#RESOURCES += gameFiles.qrc

QMAKE_CXXFLAGS -= -O2
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_LFLAGS_RELEASE += -static-libgcc -static-libstdc++ -lpthread
