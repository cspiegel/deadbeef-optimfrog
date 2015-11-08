PREFIX=	/usr
TARGET=	cas_ofr
SRCS=	ofr.cpp frogwrap.cpp
CXX=	g++
OPT=	-O2

PKG=	optimfrog

include compiler.mk
include deadbeef.mk

ofr.o: copyright.c frogwrap.h
frogwrap.o: frogwrap.h
