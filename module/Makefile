SOURCE=mod_parrot.c mod_parrot_io.c mod_parrot_run.c
MODULE=mod_parrot.la

include ../config.mk

mod_parrot.so:  $(MODULE) 
	$(LIBTOOL) --mode=install cp mod_parrot.la $(BUILDDIR)/build

$(MODULE): $(SOURCE)
		$(APXS) $(FLAGS) -c $(SOURCE)

%.c: mod_parrot.h

clean:
	rm -rf .libs *.lo *.slo *.la *.o *.a *.pbc *.so


