# So much to do!

- loading hll compilers and executing them (see mod_bart)
    This has hanged on the winxed compiler failing in embedded context, not
	sure why. It will be debugged

- execption and error logging 
	as I've found out, this can be done in winxed. It's easy to keep it there, but
	that would mean that everybody who would supply their own 'loader' pbc would
	have to handle errors themselves as well or possibly risk crashing the interpreter.
	And not have pretty errors. 

- cleanup makefile (use the relation between .lo and .c) This means
	figuring out libtool a bit more, and apxs. The advantage of my current
	(lazy) method is that apxs does all the work. The disadvantage is that
	I need to build the /whole/ thing every time I change one
	file. mod_parrot is small - and intends to stay that way - but it is
	still maddingly inelegant.

- read headers from output

- create a 'real' test framework system, possibly based on mod_perl (as it
	can manipulate apache internals), but I'm not at all picky. I could
	live with all tests passing through HTTP, as was my original plan; but
	I do want a usable way to check input and output.

- figure out if I can 'read' the apache structs from inside parrot (instead
  of manipulating parrot pmcs from inside C, as I do now), which would have
  the advantage of keeping mod_parrot small.

- if the beforementioned does not work out I'm fairly sure I can make a
  custom PMC calling apache functions. Any custom PMC's should be installed
  alongside mod_parrot.so and thus the install directory should also be
  added to the search path. Luckily, apxs knows the destined install
  directory for modules.

- rewrite mod_parrot_io using NCI to call it directly
