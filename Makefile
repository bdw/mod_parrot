all: 
	make -C module
	make -C loader

clean:
	make -C module clean
	make -C loader clean

debug: all
	@perl -Ipudding pudding/debug.pl

test: all
	@perl -Ipudding pudding/echo.pl
	@perl -Ipudding pudding/cgi.pl
