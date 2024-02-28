readme.md {#mainpage}
=========

A small lisp interpreter library and wrapper written in C. The documentation 
is in [doc/liblisp.md][], this contains information on how to build and run the
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
  - <mailto:howe.r.j.89@gmail.com>
* Repo:
  - <https://github.com/howerj/liblisp>

This project should probably be rewritten when I get around to it, as the
current project:

* Does not have Macros
* Does not have Closures (and it would be difficult to add them)
* Is not particularly memory efficient, and could be made to be more
so (and portably) by using indexing instead of pointers.
* Is not image based, so saving and restoring state is not possible,
again using indexing instead of pointers would help.

There are a few interesting things in this project but I would do it
differently if I got the chance to, both my understanding of lisp has improved
and my C. I would probably use a more liberal license as well.

The rewrite will be my forth attempt, if I get around to it...

To test this application out:

	make run

This should build under Linux, or using MinGW under Windows .

or

	make modules run

(which is more likely to fail as it requires installed dependencies).

[doc/liblisp.md]: doc/liblisp.md
[introduction]: https://wiki.debian.org/IntroDebianPackaging
