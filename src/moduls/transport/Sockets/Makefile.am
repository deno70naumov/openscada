EXTRA_DIST = *.h po/*

if SocketsIncl
noinst_LTLIBRARIES = tr_Sockets.la
tr_Sockets_la_CXXFLAGS = -DMOD_INCL -fpic
tr_Sockets_la_LIBTOOLFLAGS = --tag=disable-shared
tr_Sockets_la_LDFLAGS = -module
else
oscd_modul_LTLIBRARIES = tr_Sockets.la
tr_Sockets_la_LIBTOOLFLAGS = --tag=disable-static
tr_Sockets_la_LDFLAGS = -module -avoid-version $(LIB_MOD)
endif

tr_Sockets_la_SOURCES = socket.cpp

I18N_mod = $(oscd_modulpref)Sockets
include ../../../../I18N.mk
