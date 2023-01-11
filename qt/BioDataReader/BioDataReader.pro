QT       += core gui widgets charts
CONFIG += c++17

SOURCES += \
    EDF/EDFReader.cpp \
    EDF/edflib/edflib.c \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    EDF/EDFReader.h \
    EDF/edflib/edflib.h \
    MainWindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
