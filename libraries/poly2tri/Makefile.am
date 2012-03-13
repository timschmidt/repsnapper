noinst_LTLIBRARIES += libpoly2tri.la

libpoly2tri_la_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir)/libraries/poly2tri/poly2tri/poly2tri/ \
        $(EXTRA_CFLAGS)

libpoly2tri_la_SOURCES = \
	libraries/poly2tri/poly2tri/poly2tri/common/shapes.cc \
	libraries/poly2tri/poly2tri/poly2tri/sweep/advancing_front.cc \
	libraries/poly2tri/poly2tri/poly2tri/sweep/cdt.cc \
	libraries/poly2tri/poly2tri/poly2tri/sweep/sweep.cc \
	libraries/poly2tri/poly2tri/poly2tri/sweep/sweep_context.cc \
	libraries/poly2tri/poly2tri/poly2tri/sweep/sweep_context.h \
	libraries/poly2tri/poly2tri/poly2tri/sweep/cdt.h \
	libraries/poly2tri/poly2tri/poly2tri/sweep/advancing_front.h \
	libraries/poly2tri/poly2tri/poly2tri/sweep/sweep.h \
	libraries/poly2tri/poly2tri/poly2tri/common/shapes.h \
	libraries/poly2tri/poly2tri/poly2tri/common/utils.h \
	libraries/poly2tri/poly2tri/poly2tri/poly2tri.h
