# bignum.md

A small bignum library. I would like to get the API correct before working on
the efficiency of the code. In its current form it is not particularly
efficient and is experimental.

## New API

A possible new bignum library interface:

        typedef enmueration{
                bigop_invalid = 0,
                bigop_add = '+',
                bigop_sub = '-',
                bigop_mul = '*',
                bigop_div = '/',
                bigop_and = '&',
                bigop_or  = '|',
                bigop_xor = '^'
        } bignum_op_e 

        typedef enmueration{ /*unary operations*/
                biguop_not = '!',
                biguop_inv = '~',
                biguop_neg = '-'
        } bignum_op_e 

        int bignum_to_int(bignum_t a);
        bignum_t bignum_from_int(int a);
        bignum_t bignum_op(bigop_e op, bignum_t a, bignum_t b);
        bignum_t bignum_uop(biguop_e uop, bignum_t b);

