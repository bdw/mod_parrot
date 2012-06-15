include ../config.mk
FILES=mod_parrot.pbc apache.pbc
OUTPUT=../build/mod_parrot.pbc ../build/apache.pbc

$(OUTPUT): $(FILES)
	cp $(FILES) $(BUILDDIR)/build

%.pbc: %.pir
	parrot -o $@ $< 

%.pir: %.winxed
	winxed -c $<

clean:
	rm -f *.pir *.pbc
