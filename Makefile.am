## Any directories that you want built and installed should go here.
SUBDIRS = include src test apps

## Any directories you want a part of the distribution should be listed
## here, as well as have a Makefile generated at the end of configure.in
##
## This only works for subdirectories one level deep.
DIST_SUBDIRS = $(SUBDIRS) contrib music_sample

# All the rest of the distributed files
EXTRA_DIST = bootstrap 

# Run ldconfig after installing the library:
install-exec-hook:
	-ldconfig

README: README.md
	cp -f -p README.md README

ChangeLog:
	git log > $@
