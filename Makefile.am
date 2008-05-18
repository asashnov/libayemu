## Any directories that you want built and installed should go here.
SUBDIRS = src include

## Any directories you want a part of the distribution should be listed
## here, as well as have a Makefile generated at the end of configure.in
##
## This only works for subdirectories one level deep.
DIST_SUBDIRS = $(SUBDIRS)

# All the rest of the distributed files
EXTRA_DIST = libayemu.spec Doxyfile bootstrap RELEASENOTES vtx_format.txt

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

# Run ldconfig after installing the library:
install-exec-hook:
	-ldconfig

# create distribute .tar.gz (source archive with autotools and ChangeLog files)
#dist:
#	tar czf $(PACKAGE)-$(VERSION).tar.gz --dereference $(DIST_SUBDIRS) $(EXTRA_DIST)
