include config.mk

TARGET = netti_rcs

SRC != ls *.c
OBJ = ${SRC:.c=.o}

all: ${TARGET}

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

${TARGET}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f ${TARGET} ${TARGET}.core ${OBJ}

dist: clean
	mkdir -p ${TARGET}-${VERSION}
	cp -R RCS Makefile config.mk ${SRC} *.h ${TARGET}-${VERSION}
	tar -cf ${TARGET}-${VERSION}.tar ${TARGET}-${VERSION}
	gzip ${TARGET}-${VERSION}.tar
	rm -rf ${TARGET}-${VERSION}

install: ${TARGET}
	install -o www -g www -m 0500 ${TARGET} /var/www/cgi-bin/
	rcctl -f restart httpd

.PHONY: all clean run dist install
