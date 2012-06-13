Architecture of mod_parrot
==========================

Now that I've written 'architecture', I realise 'anatomy' sounds much
cooler. But one can not have all. The design I had in mind is based upon a
number of assumptions listed here:

* Interpreter instantiation is expensive
* The parrot interpreter is not thread-safe (the library as a whole is, though)
* The parrot bytecode is not a stable format
* The compiler API is relatively stable
* Compilation is slow
* The parrot C api is clunky (and relatively slow)
* It is relatively tricky to get data out of C structs from within parrot
* Buffering and copying the output of a script is expensive and redundant
* It is not our job to parse query parameters or POST data 
* But we should report errors and log data

The first two assumptions mean that we would like a thread-local
interpreter instance. I am not aware of the 'right' way to do this in the
apache web server, but I would be rather surprised if there was none. For
the time being, I can assume that workers are single-threaded processes, as
I am developing on linux, and the prefork multiprocess module is common
there (which is single-threaded per process). (Also, some people - in some
enviroments - might actually want to run using a new instance every
request. We should not punish them for this, and it should be an option).

Assumption 3 means that I do not expect people to precompile their
programs before putting them online (as they do with ASP.NET and probably
some other frameworks). Some people will no doubt distribute or serve their
programs as PBC files (and mod_parrot should support this workflow) but in
all likelihood most users will upload their scripts as source code. Also,
parrot iterates rapidly and the bytecode format may change. However, as a
temporary format, I expect the same script to be executed faster when
stored in PBC format than in the original source format, as compiling is
slow. (This is an excellent assumption to test by benchmark, by the way).

Assumption 4 is crucial to the implementation of so-called *bootstrapping
scripts*, which are programs written in winxed with the goal of executing a
given script within a certain enviroment. Using different bootstrapping
scripts, mod_parrot could provide different enviroments for web
applications to run in. The two most obvious examples are PSGI and CGI,
which have similar but not equal execution enviroments. Of course,
different bootstrapping scripts will probably share a lot of code. 

The fifth assumption means that it would be wise - in many cases - to
cache compiled bytecode files so as to skip compilation. Note that this
does not mean we can skip loading the language enviroment, as this may
still be necessary.

Assumption six and seven (as well as nine, in a way) concern the balance
between doing much in C versus doing much from within parrot, using
winxed. The goal is to keep each job as simple as possible, so the idea was
to convert the relevant parts of the request_rec structure into a hash
which is passed - alongside the apache options, did I mention apache module
options? They will be stored in a hash and passed as well - to the
bootstrapping script. The bootstrapping script can then setup the
enviroment. For the most part, this hash will contain a simple
string->string mapping. However, this hash will also contain the
request_rec structure itself, since it is a filehandle of sorts. More on
that below.

The eight assumption concerns output. As of apache 2.0, it is relatively
tricky to get the actual on-the-wire socket because of the filtering
API. Thus, output is buffered on the server side in 'buckets'. The good
thing is that this is relatively transparent to the user if that user is a
C programmer and cares to call the right functions. These functions take
the request_rec structure in the place of a regular filehandle and are call
ap_puts and ap_write or similar. The best solution AFAIK is to replace
parrots standard IO handles with a PMC that calls these functions, using the
request_rec structure. I will have to discuss this with whiteknight. If
this method should fail, it is still possible to return from the
bootstrapping script to the C module the output as a string (or series of
strings, because headers need to be handled separately). This would mean
buffering output thrice (from parrot, to mod_parrot, to apache), which is
memory-inefficient as well as slow, and prohibits HTTP streaming.

The 10th and last assumption means that mod_parrot needs to report errors
to the apache error log. The level of detail desired may be configured at
the module level. Within the parrot interpreter, standard error will do
nicely for this purpose.

Structure
=========

All source code goes to src/
The 'module' (C code) goes to src/module
The loaders (winxed) go to src/loader
The library for loaders go to src/library

Test to go t/

