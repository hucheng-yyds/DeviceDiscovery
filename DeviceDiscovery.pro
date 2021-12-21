QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dialoghwconfig.cpp \
    dialognwconfig.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    dialoghwconfig.h \
    dialognwconfig.h \
    mainwindow.h \
    typedef.h

FORMS += \
    dialoghwconfig.ui \
    dialognwconfig.ui \
    mainwindow.ui

#INCLUDEPATH += $$PWD/include

#LIBS += -L$$PWD/lib -lVLCQtCore -lVLCQtWidgets

INCLUDEPATH += Z:/hucheng/Documents/simplest_libvlc_example/libvlc_include
LIBS += -LZ:/hucheng/Documents/simplest_libvlc_example/libvlc_lib -lvlc -lvlccore

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
