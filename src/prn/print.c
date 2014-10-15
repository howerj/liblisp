/* Test bench for a color/standard width printf */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "color.h"

void printc(char *fmt, ...)
{
        va_list ap;
        int d;
        char c, *s;

        va_start(ap, fmt);
        while (*fmt){
                char f;
                if('%' == (f = *fmt++)){
                        switch (*fmt++) {
                        case '\0':
                                return;
                        case '%':
                                fputc('%',stdout);
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
                        putchar(f);
                }
        }
        va_end(ap);
}

int main(void)
{
        printc("%r%s%b%d%y%c%t\n", "hello", 10, 'c');
        return 0;
}
