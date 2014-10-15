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
        printc("%s%d%c", "hello", 10, 'c');
        return 0;
}
