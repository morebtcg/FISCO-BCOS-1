CLEANFILES = *~
maintainer-clean-local:
	-rm -rf Makefile.in depcomp ltmain.sh config.h.in configure compile config.guess config.sub \
		missing install-sh autom4te.cache aclocal.m4

ChangeLog :
	cg-log > ChangeLog

AM_CPPFLAGS = -Iinclude

lib_LTLIBRARIES = libpbc_sig.la

library_includedir = $(includedir)/pbc
library_include_HEADERS = include/pbc_sig.h

libpbc_sig_la_SOURCES = sig/hash.c sig/sig.c sig/bbs.c
libpbc_sig_la_CPPFLAGS = -Iinclude
libpbc_sig_la_LDFLAGS = -lpbc -lgmp -lm

noinst_PROGRAMS = test/bls test/bb test/ibs test/bbs
test_bls_CPPFLAGS = -I include
test_bls_LDADD = libpbc_sig.la
test_bls_LDFLAGS = -lpbc
test_bls_SOURCES = test/bls.c
test_bbs_CPPFLAGS = -I include
test_bbs_LDADD = libpbc_sig.la
test_bbs_LDFLAGS = -lpbc
test_bbs_SOURCES = test/bbs.c
test_bb_CPPFLAGS = -I include
test_bb_LDADD = libpbc_sig.la
test_bb_LDFLAGS = -lpbc
test_bb_SOURCES = test/bb.c
test_ibs_CPPFLAGS = -I include
test_ibs_LDADD = libpbc_sig.la
test_ibs_LDFLAGS = -lpbc
test_ibs_SOURCES = test/ibs.c
