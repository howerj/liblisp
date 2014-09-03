/**
 *  @file           color.h
 *  @brief          ANSI Escape sequences, header only
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details        
 *
 *  ANSI escape sequences, for colorizing the output.
 *
 */

#define ANSI_RESET          "\x1b[0m"
#define ANSI_BOLD_TXT       "\x1b[1m"
#define ANSI_UNDERLINE_TXT  "\x1b[4m"
#define ANSI_REVERSE_VIDEO  "\x1b[7m"
#define ANSI_COLOR_BLACK    "\x1b[30m"
#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_MAGENTA  "\x1b[35m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_WHITE    "\x1b[37m"
