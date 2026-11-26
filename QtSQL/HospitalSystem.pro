QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = HospitalSystem
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    idatabase.cpp \
    sexdelegate.cpp \
    masterview.cpp \
    loginview.cpp \
    welcomeview.cpp \
    patientview.cpp \
    patienteditview.cpp \
    doctorview.cpp \
    departmentview.cpp \
    historyview.cpp

HEADERS += \
    idatabase.h \
    sexdelegate.h \
    masterview.h \
    loginview.h \
    welcomeview.h \
    patientview.h \
    patienteditview.h \
    doctorview.h \
    departmentview.h \
    historyview.h

FORMS += \
    masterview.ui \
    loginview.ui \
    welcomeview.ui \
    patientview.ui \
    patienteditview.ui \
    doctorview.ui \
    departmentview.ui \
    historyview.ui

DISTFILES += \
    hospital.sql \
    hospital.db
