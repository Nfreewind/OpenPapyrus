/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * The new form of this macro (check if the a*b == 0) was suggested by Colin
 * Plumb <colin@nyx10.cs.du.edu>
 */
/* Removal of the inner if from from Wei Dai 24/4/96 */
#define idea_mul(r, a, b, ul) \
	ul = (unsigned long)a*b; \
	if(ul != 0) { \
		r = (ul&0xffff)-(ul>>16); \
		r -= ((r)>>16);	\
	} \
	else \
		r = (-(int)a-b+1);  /* assuming a or b is 0 and in range */

/*
 * 7/12/95 - Many thanks to Rhys Weatherley <rweather@us.oracle.com> for
 * pointing out that I was assuming little endian byte order for all
 * quantities what idea actually used bigendian.  No where in the spec does
 * it mention this, it is all in terms of 16 bit numbers and even the example
 * does not use byte streams for the input example :-(. If you byte swap each
 * pair of input, keys and iv, the functions would produce the output as the
 * old version :-(.
 */

/* NOTE - c is not incremented as per n2l */
#define n2ln(c, l1, l2, n) { \
		c += n;	\
		l1 = l2 = 0; \
		switch(n) { \
			case 8: l2 = ((unsigned long)(*(--(c)))); \
			case 7: l2 |= ((unsigned long)(*(--(c))))<< 8; \
			case 6: l2 |= ((unsigned long)(*(--(c))))<<16; \
			case 5: l2 |= ((unsigned long)(*(--(c))))<<24; \
			case 4: l1 = ((unsigned long)(*(--(c)))); \
			case 3: l1 |= ((unsigned long)(*(--(c))))<< 8; \
			case 2: l1 |= ((unsigned long)(*(--(c))))<<16; \
			case 1: l1 |= ((unsigned long)(*(--(c))))<<24; \
		} \
}

/* NOTE - c is not incremented as per l2n */
#define l2nn(l1, l2, c, n) { \
		c += n;	\
		switch(n) { \
			case 8: *(--(c)) = (unsigned char)(((l2)    )&0xff); \
			case 7: *(--(c)) = (unsigned char)(((l2)>> 8)&0xff); \
			case 6: *(--(c)) = (unsigned char)(((l2)>>16)&0xff); \
			case 5: *(--(c)) = (unsigned char)(((l2)>>24)&0xff); \
			case 4: *(--(c)) = (unsigned char)(((l1)    )&0xff); \
			case 3: *(--(c)) = (unsigned char)(((l1)>> 8)&0xff); \
			case 2: *(--(c)) = (unsigned char)(((l1)>>16)&0xff); \
			case 1: *(--(c)) = (unsigned char)(((l1)>>24)&0xff); \
		} \
}

#undef n2l
#define n2l(c, l)        (l = ((unsigned long)(*((c)++)))<<24L,	\
	    l |= ((unsigned long)(*((c)++)))<<16L, \
	    l |= ((unsigned long)(*((c)++)))<< 8L, \
	    l |= ((unsigned long)(*((c)++))))

#undef l2n
#define l2n(l, c)        (*((c)++) = (unsigned char)(((l)>>24L)&0xff), \
	    *((c)++) = (unsigned char)(((l)>>16L)&0xff), \
	    *((c)++) = (unsigned char)(((l)>> 8L)&0xff), \
	    *((c)++) = (unsigned char)(((l)     )&0xff))

#undef s2n
#define s2n(l, c) (*((c)++) = (unsigned char)(((l)     )&0xff), *((c)++) = (unsigned char)(((l)>> 8L)&0xff))
#undef n2s
#define n2s(c, l) (l = ((IDEA_INT)(*((c)++)))<< 8L, l |= ((IDEA_INT)(*((c)++)))      )

#define E_IDEA(num) \
	x1 &= 0xffff; \
	idea_mul(x1, x1, *p, ul); p++; \
	x2 += *(p++); \
	x3 += *(p++); \
	x4 &= 0xffff; \
	idea_mul(x4, x4, *p, ul); p++; \
	t0 = (x1^x3)&0xffff; \
	idea_mul(t0, t0, *p, ul); p++; \
	t1 = (t0+(x2^x4))&0xffff; \
	idea_mul(t1, t1, *p, ul); p++; \
	t0 += t1; \
	x1 ^= t1; \
	x4 ^= t0; \
	ul = x2^t0; /* do the swap to x3 */ \
	x2 = x3^t1; \
	x3 = ul;
