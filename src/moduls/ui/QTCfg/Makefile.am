if QTCfgIncl
noinst_LTLIBRARIES = ui_QTCfg.la
ui_QTCfg_la_CXXFLAGS = -DMOD_INCL -fpic
ui_QTCfg_la_LIBTOOLFLAGS = --tag=disable-shared
ui_QTCfg_la_LDFLAGS = -module
else
oscd_modul_LTLIBRARIES = ui_QTCfg.la
ui_QTCfg_la_CXXFLAGS =
ui_QTCfg_la_LIBTOOLFLAGS = --tag=disable-static
ui_QTCfg_la_LDFLAGS = -module -avoid-version $(LIB_MOD)
endif

ui_QTCfg_la_CXXFLAGS += $(QtGui_CFLAGS)
ui_QTCfg_la_LDFLAGS += $(QtGui_LIBS)
ui_QTCfg_la_SOURCES = tuimod.cpp qrc_qtcfg.cpp qtcfg.cpp selfwidg.cpp moc_qtcfg.cpp moc_selfwidg.cpp ../QTStarter/lib_qtgen.cpp moc_lib_qtgen.cpp
ui_QTCfg_la_DIST = tuimod.cpp qtcfg.cpp selfwidg.cpp qtcfg.qrc

qrc_qtcfg.cpp: qtcfg.qrc
	$(Qt_RCC) -name QTCfg qtcfg.qrc -o qrc_qtcfg.cpp
moc_qtcfg.cpp: qtcfg.h
	$(Qt_MOC) qtcfg.h > moc_qtcfg.cpp
moc_selfwidg.cpp: selfwidg.h
	$(Qt_MOC) selfwidg.h > moc_selfwidg.cpp
moc_lib_qtgen.cpp: ../QTStarter/lib_qtgen.h
	$(Qt_MOC) ../QTStarter/lib_qtgen.h > moc_lib_qtgen.cpp

I18N_mod = $(oscd_modulpref)QTCfg
include ../../../../I18N.mk

clean-local:
	rm -f qrc_qtcfg.cpp moc_qtcfg.cpp moc_selfwidg.cpp moc_lib_qtgen.cpp

EXTRA_DIST = *.h qtcfg.qrc images/* po/*
DISTFILES = $(DIST_COMMON) $(ui_QTCfg_la_DIST) $(EXTRA_DIST)
