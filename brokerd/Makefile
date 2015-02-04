# Copyright (c) 2015 Andre de Oliveira <deoliveirambx@googlemail.com>
# 
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

QTDIR?=/opt/qt486
QMAKE=${QTDIR}/bin/qmake

PROG=		brokerd

SOURCES=	brokerd.cpp engine.cpp

HEADERS=	brokerd.h engine.h

CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=	-Wmissing-declarations
CFLAGS+=	-Wpointer-arith
CFLAGS+=	-Wsign-compare

all: ${PROG}

${PROG}: ${PROG}.makefile
	${MAKE} -f $<

$(PROG).makefile: $(PROG).pro
	${QMAKE} -makefile -t app -o $@ $<

$(PROG).pro: Makefile
	${QMAKE} -project -nopwd -t app -o $@ \
	'SOURCES = $(SOURCES)' \
	'HEADERS = $(HEADERS)' \
	'TARGET = $(PROG)' \
	'QMAKE_CXXFLAGS += $(CFLAGS)' \
	'OBJECTS_DIR = .obj' \
	'MOC_DIR = .moc' \
	'CONFIG += debug' \
	'QT -= gui' \
	'QT += network' \
	@echo "include(./qtservice/qtservice.pri)" >> $@

.PHONY: distclean
distclean: clean
	-${MAKE} -f $(PROG).makefile distclean || echo "no ${PROG}.makefile"
	-rm -f $(PROG).pro
	-rm -rf .moc
	-rm -rf .obj

.PHONY: clean
clean:
	-${MAKE} -f $(PROG).makefile clean || echo "no ${PROG}.makefile"
