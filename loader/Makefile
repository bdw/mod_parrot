include ../config.mk
FILES=cgi.pbc echo.pbc apache.pbc common.pbc psgi.pbc
OUTPUT=../build/echo.pbc ../build/apache.pbc ../build/cgi.pbc ../build/psgi.pbc ../build/common.pbc

$(OUTPUT): $(FILES)
	cp $(FILES) $(BUILDDIR)/build

%.pbc: %.pir
	parrot -o $@ $< 

%.pir: %.winxed
	winxed -c $<

clean:
	rm -f *.pir *.pbc
