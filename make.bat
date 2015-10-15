@echo off
REM ==========================================================================
REM This is a minimal script to build the lisp interpreter under Windows.
REM For Unix systems use GNU Make with the file "makefile" instead.
REM See :help for more information.
REM ==========================================================================

REM == Environment variables =================================================

set CFLAGS=-Wall -shared
set CC=tcc
set MKDEF=tiny_impdef
set INSTALLDIR=C:\lisp
REM include arguments when building modules
set MINC=-I. liblisp.def

REM == Build system driver ===================================================

REM initial processing of arguments
:start
@echo off
if %1.==.              goto :modules
if "%1" == "all"       goto :modules
if "%1" == "build"     goto :build
if "%1" == "lisp"      goto :build
if "%1" == "clean"     goto :clean
if "%1" == "run"       goto :run
if "%1" == "modules"   goto :modules
if "%1" == "help"      goto :help
if "%1" == "install"   goto :install
if "%1" == "uninstall" goto :uninstall
echo unknown option: "%1"
goto :help

REM Subroutines will jump here once they are done processing their
REM commands, the next argument to make.bat will then be processed.
:procargs
@echo off
shift /1
if %1.==. goto :end
goto :start

REM Subroutines will jump here on error, the make.bat file will then
REM exit.
:error
echo failed with error #%errorlevel%.
goto :end

REM Exit the script
:end
pause
exit /b %errorlevel%

REM == Compilation ===========================================================

REM build the interpreter or fail
:build
@echo on
REM build lisp interpreter, the main object file
%CC% %CFLAGS% -DCOMPILING_LIBLISP lisp.c hash.c io.c util.c subr.c eval.c gc.c print.c repl.c read.c tr.c valid.c -o liblisp.dll  || goto :error
%MKDEF% liblisp.dll -o liblisp.def
%CC% -DUSE_DL -Wall main.c liblisp.def -o lisp.exe || goto :error
goto :procargs

REM build as many modules as possible, depends on liblisp.def
:modules
@echo off
REM This call is a hack! And it does not work correctly!
if not exist liblisp.def call :build 
REM if it still does not exist there must have been a problem
if not exist liblisp.def exit /b %errorlevel%
echo Compiling as many modules as possible.
@echo on
%CC% %CFLAGS% %MINC% mod\crc.c mod\liblisp_crc.c -o liblisp_crc.dll 
%CC% %CFLAGS% %MINC% mod\diff.c mod\liblisp_diff.c -o liblisp_diff.dll 
%CC% %CFLAGS% %MINC% mod\tsort.c mod\liblisp_tsort.c -o liblisp_tsort.dll 
%CC% %CFLAGS% %MINC% mod\bignum.c mod\liblisp_bignum.c -o liblisp_bignum.dll 
%CC% %CFLAGS% %MINC% mod\liblisp_math.c -o liblisp_math.dll 
%CC% %CFLAGS% %MINC% mod\liblisp_tcc.c libtcc.def -o liblisp_tcc.dll 
%CC% %CFLAGS% %MINC% mod\liblisp_sql.c sqlite3.def -o liblisp_sql.dll
echo Compilation errors are expected. They should only affect a single 
echo module, the modules most likely to fail are those with external
echo dependences, such as the SQLite3 or the Tiny C Compiler modules.
goto :procargs

REM == Running the interpreter ===============================================

REM run the interpreter (and build it is necessary)
:run
if not exist lisp.exe call :build
REM if it still does not exist there must have been a problem
if not exist lisp.exe exit /b %errorlevel%
lisp.exe -p init.lsp test.lsp -
goto :procargs

REM == Miscellaneous build options ===========================================

:install
echo installing lisp, libraries and modules.
@echo on
if not exist "%INSTALLDIR%" mkdir %INSTALLDIR%
copy lisp.exe   %INSTALLDIR%
copy *.dll      %INSTALLDIR%
copy liblisp.h  %INSTALLDIR%
copy liblisp.md %INSTALLDIR%
goto :procargs

:uninstall
echo uninstalling lisp, libraries and modules.
@echo on
del %INSTALLDIR%\lisp.exe   2>NUL
del %INSTALLDIR%\*.dll      2>NUL
del %INSTALLDIR%\liblisp.h  2>NUL
del %INSTALLDIR%\liblisp.md 2>NUL
rmdir %INSTALLDIR%          2>NUL
goto :procargs

REM delete build artifacts, ignore files that do not exist
:clean
@echo off
echo Removing build artifacts {lisp.exe, *.a, *.o, *~, *.def *.dll}
del lisp.exe 2>NUL
del *.a      2>NUL
del *.o      2>NUL
del *~       2>NUL
del *.def    2>NUL
del *.dll    2>NUL
goto :procargs

REM list all options / help message
:help
@echo off
echo This is the build system for the liblisp library, modules 
echo  and executable for Windows. It uses the Tiny C Compiler
echo  {see http://bellard.org/tcc/} to compile the library, this
echo  should be on your PATH. 
echo The build system is crude, but should work, the primary
echo  makefile {for Unix systems} has not been ported, but would
echo  require GNU Make to be installed as well.
echo Notes:
echo    * Release 0.9.26 of tcc, for x86_64 has a bug in it, use
echo      the 32-bit version instead.
echo options:
echo  make           runs make all by default
echo  make run       build and run the interpreter and libraries
echo  make lisp      build the interpreter and libraries
echo  make all       build the interpreter, libraries and modules
echo  make clean     clean up the build
echo  make modules   build as many modules as possible and dependencies
echo  make help      this help message
echo  make install   install the liblisp library, interpreter and modules
echo  make uninstall uninstall the liblisp library, interpreter and modules
goto :procargs

