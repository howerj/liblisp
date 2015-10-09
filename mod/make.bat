@echo off
tcc -Wall -Wextra -I.. -shared crc.c liblisp_crc.c ..\liblisp.def -o liblisp_crc.dll 
tcc -Wall -Wextra -I.. -shared diff.c liblisp_diff.c ..\liblisp.def -o liblisp_diff.dll 
tcc -Wall -Wextra -I.. -shared tsort.c liblisp_tsort.c ..\liblisp.def -o liblisp_tsort.dll 
tcc -Wall -Wextra -I.. -shared bignum.c liblisp_bignum.c ..\liblisp.def -o liblisp_bignum.dll 
tcc -Wall -Wextra -I.. -shared liblisp_math.c  ..\liblisp.def -o liblisp_math.dll 
tcc -Wall -Wextra -I.. -shared liblisp_tcc.c  ..\liblisp.def ..\libtcc.def -o liblisp_tcc.dll 
REM tcc -Wall -Wextra -I.. -shared liblisp_sql.c ..\liblisp.def ..\sqlite3.def -o liblisp_sql.dll
copy *.dll ..\
