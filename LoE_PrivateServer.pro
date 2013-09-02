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
    dataType.cpp

HEADERS  += widget.h \
	character.h \
	message.h \
	utils.h \
    scene.h \
    dataType.h

FORMS    += widget.ui

RESOURCES += \
	gameFiles.qrc

QMAKE_CXXFLAGS += -O0
