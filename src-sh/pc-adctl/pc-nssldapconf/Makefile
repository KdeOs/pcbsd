#
#	Makefile
#

.include <bsd.own.mk>

MAN=
PROGOWN=	root
PROGGRP=	wheel
PROGMODE=	0555
DESTDIR=	/usr/local/bin

PROG=	pc-nssldapconf
SRCS=	pc-nssldapconf.c pc-nssldapconf.h

CFLAGS+= -I. -I${.CURDIR}

MAN=
DEBUG?=	0

BISON=	yacc
LEX=	lex

.if ${DEBUG} == 1
LEXFLAGS= -d
CFLAGS+=-g -ggdb -g3
.else
LEXFLAGS=
.endif

OBJS+=	nssldap-parser.tab.o nssldap-lexer.lex.o

CLEANFILES+= nssldap-parser.tab.o nssldap-lexer.lex.o \
	nssldap-parser.tab.c nssldap-parser.tab.h nssldap-lexer.lex.c \
	nssldap-parser.tab.i

nssldap-lexer.tab.o: nssldap-parser.tab.c nssldap-parser.tab.h
nssldap-parser.tab.o: nssldap-lexer.lex.c nssldap-parser.tab.h

nssldap-parser.tab.c: nssldap-parser.y
	${BISON} -o$@ -d nssldap-parser.y

nssldap-parser.tab.h: nssldap-parser.tab.c

nssldap-lexer.lex.c: nssldap-lexer.l
	${LEX} ${LEXFLAGS} -o$@ nssldap-lexer.l

.include <bsd.prog.mk>
