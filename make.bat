@echo off
REM @todo This "make" batch file needs lots of improving
REM This is a minimal script to build the lisp interpreter
REM for Windows, it uses the Tiny C Compiler (tcc) available
REM from <http://bellard.org/tcc/> which has all the C99 support
REM needed to compile the library.
tcc -DNDEBUG -DUSE_MATH liblisp.c main.c -o lisp.exe || goto :error
lisp.exe -p init.lsp test.lsp -
REM *fallthrough and report lisp.exe error status*
REM use "goto :EOF" to skip the next lines
:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%

