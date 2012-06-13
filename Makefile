all: 
	make -C module
	make -C loader

clean:
	make -C module clean
	make -C loader clean

debug:
	@perl -Ipudding pudding/debug.pl

test:
	@perl -Ipudding pudding/test.pl