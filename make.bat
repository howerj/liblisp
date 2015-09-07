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
tcc -DNDEBUG -Wall -Wextra lisp.c  -c -o lisp.o    || goto :error
tcc -DNDEBUG -Wall -Wextra hash.c  -c -o hash.o    || goto :error
tcc -DNDEBUG -Wall -Wextra io.c    -c -o io.o      || goto :error
tcc -DNDEBUG -Wall -Wextra util.c  -c -o util.o    || goto :error
tcc -DNDEBUG -Wall -Wextra subr.c  -c -o subr.o    || goto :error
tcc -DNDEBUG -Wall -Wextra eval.c  -c -o eval.o    || goto :error
tcc -DNDEBUG -Wall -Wextra gc.c    -c -o gc.o      || goto :error
tcc -DNDEBUG -Wall -Wextra print.c -c -o print.o   || goto :error
tcc -DNDEBUG -Wall -Wextra repl.c  -c -o repl.o    || goto :error
tcc -DNDEBUG -Wall -Wextra read.c  -c -o read.o    || goto :error
tcc -DNDEBUG -Wall -Wextra tr.c    -c -o tr.o      || goto :error

REM built without asserts as its easier
REM built without libline as it has not been ported to Windows
REM built with "math.h" functions added
tcc -DNDEBUG -DUSE_MATH -Wall -Wextra lisp.o hash.o io.o util.o gc.o eval.o repl.o subr.o read.o print.o tr.o main.c -o lisp.exe || goto :error
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
exit /b 0  

REM list all options
:help
echo options:
echo make run
echo make lisp
echo make all
echo make clean
exit /b 0
