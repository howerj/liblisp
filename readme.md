readme.md {#mainpage}
=========

A small lisp interpreter library and wrapper written in C. The documentation 
is in [liblisp.md][], this contains information on how to build and run the
interpreter. The project runs (and has been tested under) Linux and Windows.


         _ _ _     _ _           
        | (_) |   | (_)          
        | |_| |__ | |_ ___ _ __  
        | | | '_ \| | / __| '_ \ 
        | | | |_) | | \__ \ |_) |
        |_|_|_.__/|_|_|___/ .__/ 
                          | |    
                          |_|    


* Author:
  - Richard James Howe
* License:
  - LGPL v2.1 or later version
* Email:
  - <howe.r.j.89@gmail.com>

This project should probably be rewritten when I get around to it, as the
current project:

* Does not have Macros
* Does not have Closures (and it would be difficult to add them)
* Is not particularly memory efficient, and could be made to be more
so (and portably) by using indexing instead of pointers.
* Is not image based, so saving and restoring state is not possible,
again using indexing instead of pointers would help.

The rewrite will be my forth attempt, if I get around to it...

To test this application out:

	make run

This should build under Linux, or using MinGW under Windows .

or

	make modules run

(which is more likely to fail as it requires installed dependencies).

For packaging of this application there are two options:

1)

	make app

2) The [debian][] directory

This directory contains most of what is needed to create a Debian package, for
more details on how to build a Debian package see this [introduction][], but
this following instructions should work (tested on Debian 8):

	mv liblisp/ liblisp-0.6/
	tar zcf liblisp_0.6.orig.tar.gz liblisp-0.6/
	cd liblisp-0.6/
	debuild -us -uc
	sudo dpkg -i ../liblisp_0.6-1_XXX.deb # XXX is your architecture

[liblisp.md]: liblisp.md
[debian]: debian/
[introduction]: https://wiki.debian.org/IntroDebianPackaging
