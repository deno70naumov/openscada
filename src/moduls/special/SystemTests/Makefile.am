EXTRA_DIST = *.h po/*

oscd_modul_LTLIBRARIES = spec_SystemTests.la
spec_SystemTests_la_LDFLAGS = -module -avoid-version $(LIB_MOD)
spec_SystemTests_la_SOURCES = mod_tests.cpp
spec_SystemTests_la_LIBTOOLFLAGS = --tag=disable-static

I18N_mod = $(oscd_modulpref)SystemTests
include ../../../../I18N.mk
