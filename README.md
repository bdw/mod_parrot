mod_parrot
==========

The awesome module that will run your parrot programs from within apache!

### Prerequisites

- Apache HTTPD 2.2 or newer
- Apache APXS (included in apache-dev packages, or whatever they are called)
- pkg-config 
- parrot > 4.4.0 including parrot_config
- libicu if parrot was build with that

### Configuration

An installation helper is not yet done, if you must install it on a 'live'
system, simply make and copy the contents of ./build to the apache module
directory like so:

    cp -v ./build/* `apxs -q LIBEXECDIR`

Debian famously renames apxs to apxs2, so you will have to use that instead.
After that, edit your local httpd.conf to load and activate the module:

    LoadModule modules/mod_parrot.so

Quite probably, if you are on windows and you did get this far, you will
have to rename .so to .dll. (If you did you should contact me, and I will
celebrate the thing with you). The last thing you must edit are the
languages on which mod_parrot will respond. It currently uses a rather
simplistic mapping from filenames to languages, so you will have to add

    ParrotLanguage winxed .winxed .wxd .wnxd

to make mod_parrot respond to requests ending in .winxed, .wxd, or
.wnxd. In essence, this should work for perl6 and other supported
languages, but unhappily does not for nqp (the perl6 bootstrapping
language).

Note that I'd like to move this over to a mime-based approach, but that is
not yet done nor totally figured out.

### Loaders

mod_parrot uses scripts known as 'loaders' to determine how your program
is invoked. The default loader is the CGI loader, which emulates a CGI
enviroment inside the apache process. This loader can only ever be used in
single-threaded MPM's such as prefork. (If you do not know what this is and
are using unix, you are probably safe, however on windows this is very much
untrue). A much wiser choice is the PSGI loader. Read about the PSGI loader
on [its website](http://search.cpan.org/~miyagawa/PSGI-1.101/PSGI.pod),
which is also multi-thread compatible because it does not have to
manipulate the enviroment or change directories. As you might notice if you
are a ruby or python programmer, PSGI is nearly exactly equal to WSGI /
Rack. This is intentional, because it hopefully allows many different
language frameworks to be ported to mod_parrot with minimal effort.

The parrot PSGI implementation calls the main subroutine from the requested
file, CGI style. Silly, right, I know. mod_parrot should really have some
way to specify the PSGI entry-point, but it isn't quite there yet.

### Whats next and coming

- Run scripts in a child-interpreter. The trick here is rethrowing
  exceptions. 
- Respond to requests based on mime type (via mod_mime). The actual
  language should be determined by the contents of the file.
- Store a persistent (pool of) interpreter(s) for swiftly handling
  responses. 
- Run NQP as a language, which is troubling, and perl6, which should not be
  so, but which I can't test right now.
- Add a way to run a script upon errors (ParrotError option?) and
  preferably, upon startup (ParrotConfigure) and receiving requests
  (ParrotRoute or ParrotReceive)
- And as mentioned earlier, a way to specify the entrypoint for PSGI
  applications (ParrotEntry? That sounds pretty much alright.)
- Cache PackfileViews obtained by compilation, so scripts can run faster. 
- So much more!
