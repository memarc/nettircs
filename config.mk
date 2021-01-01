VERSION=1.0
# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = `pkg-config --cflags kcgi`
LIBS = -static `pkg-config --static --libs kcgi-html`

# flags
CFLAGS   =  -Wall -Wno-deprecated-declarations -Os ${INCS}
LDFLAGS  = ${LIBS}

# compiler and linker
CC = cc
