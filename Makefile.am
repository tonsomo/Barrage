SUBDIRS = src

AUTOMAKE_OPTIONS = foreign
 
RPMBUILD = rpmbuild
rpm: dist
	$(RPMBUILD) -ta $(distdir).tar.gz

desktopentrydir = $(datadir)/applications
desktopentry_DATA = $(PACKAGE).desktop

EXTRA_DIST = BUGS README COPYING AUTHORS ChangeLog INSTALL NEWS TODO \
	$(PACKAGE).spec $(PACKAGE).spec.in $(desktopentry_DATA)

