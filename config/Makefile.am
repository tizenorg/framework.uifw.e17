MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = default samsung

EET_EET = @eet_eet@

EXTRA_DIST = profile.src

filesdir = $(datadir)/enlightenment/data/config

files_DATA = profile.cfg

SUFFIXES = .cfg

.src.cfg:
	$(EET_EET) -i \
	$(top_builddir)/config/$@ config \
	$< 0

clean-local:
	rm -rf *.cfg *~
