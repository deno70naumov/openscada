if VisionIncl
noinst_LTLIBRARIES = ui_Vision.la
ui_Vision_la_CXXFLAGS = -DMOD_INCL -fpic
ui_Vision_la_LIBTOOLFLAGS = --tag=disable-shared
ui_Vision_la_LDFLAGS = -module
else
oscd_modul_LTLIBRARIES = ui_Vision.la
ui_Vision_la_CXXFLAGS =
ui_Vision_la_LIBTOOLFLAGS = --tag=disable-static
ui_Vision_la_LDFLAGS = -module -avoid-version $(LIB_MOD)
endif

ui_Vision_la_CXXFLAGS += $(QtGui_CFLAGS) $(QtWebEngine_CFLAGS) $(QtMultimediaEngine_CFLAGS)
ui_Vision_la_LDFLAGS += $(QtGui_LIBS) $(LIB_FFTW3) $(QtWebEngine_LIBS) $(QtMultimediaEngine_LIBS)
ui_Vision_la_SOURCES = tvision.cpp qrc_vision.cpp vis_widgs.cpp moc_vis_widgs.cpp vis_shapes.cpp moc_vis_shapes.cpp \
    vis_devel.cpp moc_vis_devel.cpp vis_devel_dlgs.cpp moc_vis_devel_dlgs.cpp vis_devel_widgs.cpp moc_vis_devel_widgs.cpp \
    vis_run.cpp moc_vis_run.cpp vis_run_widgs.cpp moc_vis_run_widgs.cpp vis_shape_elfig.cpp moc_vis_shape_elfig.cpp ../QTStarter/lib_qtgen.cpp moc_lib_qtgen.cpp
ui_Vision_la_DIST = tvision.cpp vis_widgs.cpp vis_shapes.cpp vis_devel.cpp vis_devel_dlgs.cpp vis_devel_widgs.cpp \
    vis_run.cpp vis_run_widgs.cpp vis_shape_elfig.cpp vision.qrc

qrc_vision.cpp: vision.qrc
	$(Qt_RCC) -name Vision vision.qrc -o qrc_vision.cpp
moc_vis_devel.cpp: vis_devel.h
	$(Qt_MOC) vis_devel.h > moc_vis_devel.cpp
moc_vis_devel_widgs.cpp: vis_devel_widgs.h
	$(Qt_MOC) vis_devel_widgs.h > moc_vis_devel_widgs.cpp
moc_vis_devel_dlgs.cpp: vis_devel_dlgs.h
	$(Qt_MOC) vis_devel_dlgs.h > moc_vis_devel_dlgs.cpp
moc_vis_widgs.cpp: vis_widgs.h
	$(Qt_MOC) vis_widgs.h > moc_vis_widgs.cpp
moc_vis_run.cpp: vis_run.h
	$(Qt_MOC) vis_run.h > moc_vis_run.cpp
moc_vis_run_widgs.cpp: vis_run_widgs.h
	$(Qt_MOC) vis_run_widgs.h > moc_vis_run_widgs.cpp
moc_vis_shapes.cpp: vis_shapes.h
	$(Qt_MOC) vis_shapes.h > moc_vis_shapes.cpp
moc_vis_shape_elfig.cpp: vis_shape_elfig.h
	$(Qt_MOC) vis_shape_elfig.h > moc_vis_shape_elfig.cpp
moc_lib_qtgen.cpp: ../QTStarter/lib_qtgen.h
	$(Qt_MOC) ../QTStarter/lib_qtgen.h > moc_lib_qtgen.cpp

I18N_mod = $(oscd_modulpref)Vision
include ../../../../I18N.mk

clean-local:
	rm -f qrc_vision.cpp moc_vis_widgs.cpp moc_vis_devel.cpp moc_vis_devel_widgs.cpp moc_vis_devel_dlgs.cpp \
    moc_vis_run.cpp moc_vis_run_widgs.cpp moc_vis_shapes.cpp moc_vis_shape_elfig.cpp moc_lib_qtgen.cpp

EXTRA_DIST = *.h vision.qrc images/* po/*
DISTFILES = $(DIST_COMMON) $(ui_Vision_la_DIST) $(EXTRA_DIST)
