@echo off
REM This is a minimal script to build the lisp interpreter
REM for Windows, it uses the Tiny C Compiler (tcc) available
REM from <http://bellard.org/tcc/> which has all the C99 support
REM needed to compile the library.
REM
REM This consists of a bunch of hacks to work around the Windows
REM command processors unique outlook on life. I should instead
REM make the projects makefile work with Windows as well.
REM

REM "shift" could be used here to process multiple arguments
REM and "call" instead of "goto", error checking would have to
REM added as well.
if %1.==. goto :help
if "%1" == "run" goto :run
if "%1" == "lisp" goto :build
if "%1" == "build" goto :build
if "%1" == "clean" goto :clean
if "%1" == "all" goto :build
echo unknown option: "%1"
goto :help

REM build the interpreter or fail
:build
@echo on
REM build lisp interpreter, the main object file
tcc -DNDEBUG -DCOMPILING_LIBLISP -Wall -Wextra -shared lisp.c hash.c io.c util.c subr.c eval.c gc.c print.c repl.c read.c tr.c valid.c -o liblisp.dll  || goto :error
tiny_impdef liblisp.dll -o liblisp.def
tcc -DUSE_DL -DNDEBUG -Wall -Wextra main.c liblisp.def -o lisp.exe || goto :error
exit /b 0

:run
REM call build if the lisp interpreter has not been built
if NOT EXIST lisp.exe call :build
REM if it still does not exist there must have been a problem
if NOT EXIST lisp.exe exit /b %errorlevel%
lisp.exe -p init.lsp test.lsp -
exit /b %errorlevel%

REM goto only, exit with message
:error
echo failed with error #%errorlevel%.
exit /b %errorlevel%

REM delete build artifacts, ignore files that do not exist
:clean
del lisp.exe 2>NUL
del /q *.a   2>NUL
del /q *.o   2>NUL
del /q *~    2>NUL
del /q *.def 2>NUL
del /q *.dll 2>NUL
exit /b 0  

REM list all options
:help
echo options:
echo make run
echo make lisp
echo make all
echo make clean
exit /b 0
