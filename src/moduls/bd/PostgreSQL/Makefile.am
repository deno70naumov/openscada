EXTRA_DIST = *.h po/*

if PostgreSQLIncl
noinst_LTLIBRARIES = bd_PostgreSQL.la
bd_PostgreSQL_la_CXXFLAGS = -DMOD_INCL -fpic
bd_PostgreSQL_la_LDFLAGS = -module
else
oscd_modul_LTLIBRARIES = bd_PostgreSQL.la
bd_PostgreSQL_la_CXXFLAGS =
bd_PostgreSQL_la_LIBTOOLFLAGS = --tag=disable-static
bd_PostgreSQL_la_LDFLAGS = -module -avoid-version $(LIB_MOD)
endif

bd_PostgreSQL_la_CXXFLAGS += $(POSTGRESQL_CFLAGS)
bd_PostgreSQL_la_LDFLAGS += $(POSTGRESQL_LDFLAGS)
bd_PostgreSQL_la_SOURCES = postgre.cpp

I18N_mod = $(oscd_modulpref)PostgreSQL
include ../../../../I18N.mk
