@echo off
REM This is a minimal script to build the lisp interpreter
REM for Windows, it uses the Tiny C Compiler (tcc) available
REM from <http://bellard.org/tcc/> which has all the C99 support
REM needed to compile the library.
tcc -DUSE_MATH liblisp.c main.c -o lisp.exe || goto :error
lisp.exe -dp init.lsp test.lsp -
REM *fallthrough and report lisp.exe error status*
REM use "goto :EOF" to skip the next lines
:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%

