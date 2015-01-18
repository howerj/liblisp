#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

buf_t *cstrtobuf(char *s)
{
        size_t len;
        buf_t *buf = NULL;
        assert(s);
        len = strlen(s);
        if(NULL == (buf = calloc(1, sizeof(*buf))))
                return NULL;
        if(NULL == (buf->buf = calloc(len,sizeof(buf->buf[0]))))
                return NULL;
        memcpy(buf->buf,s,len); /*not len+1*/
        return buf;
}

char *buftocstr(buf_t *b)
{
        assert(b);
}

void bufset(buf_t *s, uint8_t c)
{
        assert(s);
        memset(s->buf,c,s->size);
}

buf_t *bufcpy(buf_t *dst, buf_t *src)
{
        assert(dst && src);
}

buf_t *bufcat(buf_t *dst, buf_t *src)
{
        assert(dst && src);
}

int  bufcmp(buf_t *a, buf_t *b)
{
        assert(a && a);
}

size_t bufspn(buf_t *s, char *accept)
{
        assert(s && accept);
}

