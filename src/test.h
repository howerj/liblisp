/**
 *  @file           test.h
 *  @brief          Utility functions for the test bench.
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 */
#ifndef TEST_H
#define TEST_H

#ifdef __cplusplus
extern "C" {
#endif

int utest(int zero_is_pass, char *msg, char *file, int line);
#define UTEST(ZERO_IS_PASS, MSG) utest((ZERO_IS_PASS),(MSG), __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif
#endif
