EXTRA_DIST = *.h libdscud.a po/*

oscd_modul_LTLIBRARIES = daq_DiamondBoards.la
daq_DiamondBoards_la_LDFLAGS = -module -avoid-version $(LIB_MOD)
daq_DiamondBoards_la_SOURCES = diamond.cpp
daq_DiamondBoards_la_LIBADD  = -L./ -ldscud
daq_DiamondBoards_la_LIBTOOLFLAGS = --tag=disable-static

I18N_mod = $(oscd_modulpref)DiamondBoards
include ../../../../I18N.mk
