#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* 
https://stackoverflow.com/questions/1590893/error-trying-to-define-a-1-024-bit-128-byte-bit-field

#define BITFIELD(SIZE, NAME) \
    unsigned char NAME[(SIZE) / CHAR_BIT + ((SIZE) % CHAR_BIT != 0)]

void setbit(unsigned char field[], size_t idx)
{ field[idx / CHAR_BIT] |= 1u << (idx % CHAR_BIT); }

void unsetbit(unsigned char field[], size_t idx)
{ field[idx / CHAR_BIT] &= ~(1u << (idx % CHAR_BIT)); }

void togglebit(unsigned char field[], size_t idx)
{ field[idx / CHAR_BIT] ^= 1u << (idx % CHAR_BIT); }

int isbitset(unsigned char field[], size_t idx)
{ return field[idx / CHAR_BIT] & (1u << (idx % CHAR_BIT)) ? 1 : 0; }*/

typedef struct {
        size_t max;
        unsigned char field[];
} bitfield;

bitfield *mkbitfield(size_t maxbits) {
        bitfield *bf;
        size_t al = (maxbits / CHAR_BIT) + !!(maxbits % CHAR_BIT);
        if(!(bf = calloc(sizeof(*bf) + al, 1))) 
                return NULL;
        bf->max = maxbits;
        return bf;
}

void setbit(bitfield *bf, size_t idx) { assert(bf && idx < bf->max);
        bf->field[idx / CHAR_BIT] |= 1u << (idx % CHAR_BIT); 
}

void unsetbit(bitfield *bf, size_t idx) { assert(bf && idx < bf->max);
        bf->field[idx / CHAR_BIT] &= ~(1u << (idx % CHAR_BIT)); 
}

void togglebit(bitfield *bf, size_t idx) { assert(bf && idx < bf->max);
        bf->field[idx / CHAR_BIT] ^= 1u << (idx % CHAR_BIT); 
}

int isbitset(bitfield *bf, size_t idx) { assert(bf && (idx < bf->max));
        return bf->field[idx / CHAR_BIT] & (1u << (idx % CHAR_BIT)) ? 1 : 0; 
}

int main(void)
{
    bitfield *foo = mkbitfield(1025);

    printf("%i", isbitset(foo, 1011));

    setbit(foo, 1011);
    printf("%i", isbitset(foo, 1011));

    unsetbit(foo, 1011);
    printf("%i\n", isbitset(foo, 1011));
    free(foo);
}
