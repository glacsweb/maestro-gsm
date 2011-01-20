SUBDIRS = src 

EXTRA_DIST = autogen.sh TODO

upload: all
	cd src && $(MAKE) upload

docs:
	@echo "running doxygen..."
	@doxygen Doxyfile
	@echo "documentation is in doc/."
