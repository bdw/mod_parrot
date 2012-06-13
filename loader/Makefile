include ../config.mk

all: mod_parrot.pbc
	cp $< $(BUILDDIR)/build

%.pbc: %.pir
	parrot -o $@ $< 
	cp $@ build

%.pir: %.winxed
	winxed -c $<

clean:
	rm -f *.pir *.pbc
