#include <stddef.h>
#include <stdint.h>

typedef struct {
        uint8_t *buf;
        size_t size; 
} buf_t;

buf_t   *cstrtobuf(char *s);
char    *buftocstr(buf_t *b);
int     bufcmp(buf_t *a, buf_t *b);
void    bufset(buf_t *s, uint8_t c);
buf_t   *bufcpy(buf_t *dst, buf_t *src);
buf_t   *bufcat(buf_t *dst, buf_t *src);
size_t  bufspn(buf_t *s, char *accept);
