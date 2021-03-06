#-------------------------------------------------
#
# Project created by QtCreator 2020-03-28T11:36:54
#
#-------------------------------------------------

QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

include(hardwareinterface/hardwareinterface.pri)
include(modbus/modbus.pri)
include(env.pri)

DESTDIR = $${PWD}/dist
TARGET = zhuosheng
TEMPLATE = app

win32 {
DEFINES += NO_STEP_CHECK
}



SOURCES += main.cpp\
    systemwindow.cpp \
    iprotocol.cpp \
    itask.cpp \
    elementinterface.cpp \
    elementfactory.cpp \
    protocolv1.cpp \
    profile.cpp \
    common.cpp \
    instructioneditor.cpp \
    qfmain.cpp \
    login/userdlg.cpp \
    login/md5.cpp \
    login/loginmanage.cpp \
    modbusmodule.cpp \
    querydata.cpp \
    calibframe.cpp \
    datafit.cpp \
    keyboard/keyboardenter.cpp \
    keyboard/keyboard.cpp \
    screensaver.cpp \
    globelvalues.cpp \
    smooth.cpp \
    calib420ma.cpp \
    usercalib.cpp \
    autocalib.cpp \
    printer.cpp \
    DataReport.cpp \
    hbprotocol.cpp \
    tntask.cpp

HEADERS  += systemwindow.h \
    iprotocol.h \
    itask.h \
    elementinterface.h \
    elementfactory.h \
    protocolv1.h \
    profile.h \
    common.h \
    instructioneditor.h \
    qfmain.h \
    login/userdlg.h \
    login/md5.h \
    login/loginmanage.h \
    modbusmodule.h \
    defines.h \
    querydata.h \
    calibframe.h \
    datafit.h \
    keyboard/keyboardenter.h \
    keyboard/keyboard.h \
    screensaver.h \
    globelvalues.h \
    smooth.h \
    calib420ma.h \
    usercalib.h \
    autocalib.h \
    printer.h \
    DataReport.h \
    hbprotocol.h \
    tntask.h

FORMS    += systemwindow.ui \
    qfmain.ui \
    login/userdlg.ui \
    modbusmodule.ui \
    setui.ui \
    querydata.ui \
    calibframe.ui \
    maintaince.ui \
    measuremode.ui \
    keyboard/keyboard.ui \
    screensaver.ui \
    lightvoltage.ui \
    calib420ma.ui \
    usercalib.ui \
    autocalib.ui \
    DataReport.ui


RESOURCES += res/qtres.qrc

OTHER_FILES += \
    SerialPort/SerialPort.pri

target.path += /dist
INSTALLS += target

