
typedef enum {
        REGEX_FAIL = -1,
        REGEX_NOMATCH = 0,
        REGEX_MATCH = 1
} regex_e;

#define REGEX_MAX_DEPTH (8192u)

typedef enum {
        false,
        true
} bool;

int regex_match(char *regexp, char *text);
