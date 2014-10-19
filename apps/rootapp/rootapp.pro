#  We need the qt libraries, we want compiler warnings off, and this is a release version of the program  
CONFIG += qt
CONFIG +=warn_off
CONFIG +=release
CONFIG +=nostrip 

#  Widgets needed for QT5, 
QT += dbus

TEMPLATE = app
TARGET = cmstroothelper
target.path = /usr/bin/
INSTALLS += target

# dbus
DBUS_ADAPTORS		+= ./code/roothelper/org.monkey_business_enterprises.roothelper.xml
DBUS_INTERFACES	+= ./code/roothelper/org.monkey_business_enterprises.roothelper.xml

#	header files
HEADERS		+= ./code/roothelper/roothelper.h

#	sources
SOURCES += ./code/main.cpp
SOURCES	+= ./code/roothelper/roothelper.cpp

##  Place all object files in their own directory and moc files in their own directory
##  This is not necessary but keeps things cleaner.
OBJECTS_DIR = ./object_files
MOC_DIR = ./moc_files

sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
