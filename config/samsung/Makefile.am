MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
        profile.desktop \
        icon.png \
	e.src 

filesdir = $(datadir)/enlightenment/data/config/samsung
files_DATA = \
        profile.desktop \
        icon.png \
	e.cfg 

SUFFIXES = .cfg

.src.cfg:
	$(EET_EET) -e \
	$(top_builddir)/config/samsung/$@ config \
	$< 1

clean-local:
	rm -rf *.cfg *~
	
