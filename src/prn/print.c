/* Test bench for a color/fixed width printf */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "color.h"

int printc(char *fmt, ...);

/**
 * @brief       A simple printf replacement that does not handle (nor need to
 *              handle) any of the advanced formatting features that make
 *              printf...printf. It handles color formatting codes as well
 *              and fixed width types (int8_t, int32_t, int64_t) but not
 *              floating point numbers.
 * @param       fmt     The formatting string
 * @param       ...     Variable length number of arguments
 * @return      int     Number of character written. You can use this to
 *                      see if ANSI color codes are supported. Negative
 *                      on error or EOF.
 *
 * format
 * %% -> %
 * %s -> string
 * %d -> int
 * %c -> char
 *
 * If enabled and feature is compiled in print the
 * ANSI escape sequence for:
 *
 * %t -> Reset
 * %z -> Reverse Video
 * %k -> Black
 * %r -> Red
 * %g -> Green
 * %y -> Yellow
 * %b -> Blue
 * %m -> Magenta
 * %a -> Cyan
 * %w -> White
 *
 * Otherwise map to <nothing>
 *
 * %<default> -> <nothing>
 * %<EOL> -> <nothing>
 *
 * <character> -> <character>
 *
 * It should return the number of characters written, but
 * does not at the moment.
 **/
int printc(char *fmt, ...)
{
        va_list ap;
        int d, count = 0;
        char c, *s;

        va_start(ap, fmt);
        while (*fmt){
                char f;
                if('%' == (f = *fmt++)){
                        switch (*fmt++) {
                        case '\0':/*we're done, finish up*/
                                goto FINISH;
                        case '%':
                                fputc('%',stdout);
                                break;
                        case 's':      
                                s = va_arg(ap, char *);
                                fputs(s,stdout);
                                break;
                        case 'd':      /* int */
                                d = va_arg(ap, int32_t);
                                printf("%d", d);
                                break;
                        case 'c':     
                                /* need a cast here since va_arg only
                                   takes fully promoted types */
                                c = (char)va_arg(ap, int);
                                putchar(c);
                                break;
#ifndef NO_ANSI_ESCAPE_SEQUENCES
                        case 't': /*reset*/
                                fputs(ANSI_RESET,stdout);
                                break;
                        case 'z': /*reverse video*/
                                fputs(ANSI_REVERSE_VIDEO,stdout);
                                break;
                        case 'k': /*blacK*/
                                fputs(ANSI_COLOR_BLACK,stdout);
                                break;
                        case 'r': /*Red*/
                                fputs(ANSI_COLOR_RED,stdout);
                                break;
                        case 'g': /*Green*/
                                fputs(ANSI_COLOR_GREEN,stdout);
                                break;
                        case 'y': /*Yellow*/
                                fputs(ANSI_COLOR_YELLOW,stdout);
                                break;
                        case 'b': /*Blue*/
                                fputs(ANSI_COLOR_BLUE,stdout);
                                break;
                        case 'm': /*Magenta*/
                                fputs(ANSI_COLOR_MAGENTA,stdout);
                                break;
                        case 'a': /*cyAn*/
                                fputs(ANSI_COLOR_CYAN,stdout);
                                break;
                        case 'w': /*White*/
                                fputs(ANSI_COLOR_WHITE,stdout);
                                break;
#endif
                        default:
                                break;
                        }
                } else {
                        count++;
                        putchar(f);
                }
        }
FINISH:
        va_end(ap);
        return count;
}

int main(void)
{
        printc("%r%s%b%d%y%c%t\n", "hello", 10, 'c');
        return 0;
}
