MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images

EDJE_CC = @edje_cc@
EDJE_FLAGS_VERBOSE_ = 
EDJE_FLAGS_VERBOSE_0 = 
EDJE_FLAGS_VERBOSE_1 = -v
EDJE_FLAGS = $(EDJE_FLAGS_$(V)) \
-id $(top_srcdir)/data/themes/images \
-fd $(top_srcdir)/data/themes/fonts \
@EDJE_DEF@

filesdir = $(datadir)/enlightenment/data/themes
files_DATA = \
samsung.edj

EXTRA_DIST = \
samsung.edc

samsung.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/samsung.edc \
	$(top_builddir)/data/themes/samsung.edj

clean-local:
	rm -f *.edj
