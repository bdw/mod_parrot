all: 
	make -C module
	make -C loader

clean:
	make -C module clean
	make -C loader clean

debug: all
	@perl -Ipudding pudding/debug.pl

test: all # todo, combine them
	@perl -Ipudding pudding/echo.pl
	@perl -Ipudding pudding/cgi.pl
	@perl -Ipudding pudding/psgi.pl
	@perl -Ipudding pudding/mime.pl