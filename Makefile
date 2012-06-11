all: 
	make -C module
	make -C loader

clean:
	make -C module clean
	make -C loader clean
