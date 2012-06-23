include ../config.mk

SOURCE=mod_parrot.c mod_parrot_io.c mod_parrot_run.c mod_parrot_util.c
MODULE=mod_parrot.la
OUTPUT=../build/mod_parrot.so

$(OUTPUT):  $(MODULE) 
	$(LIBTOOL) --mode=install cp mod_parrot.la $(BUILDDIR)/build

$(MODULE): $(SOURCE)
		$(APXS) $(FLAGS) -c $(SOURCE)

$(SOURCE): mod_parrot.h config.h

clean:
	rm -rf .libs *.lo *.slo *.la *.o *.a *.pbc *.so


