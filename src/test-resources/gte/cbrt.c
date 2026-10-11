#include "testfwk.h"
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
*/

void abort (void);
void exit (int);

#ifndef __vax__
static const unsigned long
	B1 = 715094163, /* B1 = (682-0.03306235651)*2**20 */
	B2 = 696219795; /* B2 = (664-0.03306235651)*2**20 */

static const double
	C =  5.42857142857142815906e-01, /* 19/35     = 0x3FE15F15, 0xF15F15F1 */
	D = -7.05306122448979611050e-01, /* -864/1225 = 0xBFE691DE, 0x2532C834 */
	E =  1.41428571428571436819e+00, /* 99/70     = 0x3FF6A0EA, 0x0EA0EA0F */
	F =  1.60714285714285720630e+00, /* 45/28     = 0x3FF9B6DB, 0x6DB6DB6E */
	G =  3.57142857142857150787e-01; /* 5/14      = 0x3FD6DB6D, 0xB6DB6DB7 */

#ifdef __cc45__
/* cc45: double is the CBM 40-bit float. Byte 0 is the exponent (the value
   is 1.f * 2^(exp-129)), bit 7 of byte 1 the sign, then 31 fraction bits.
   The same algorithm on that layout: the first estimate divides the
   exponent with the top 7 fraction bits (w) by 3, like the high word of an
   IEEE double above; 86 = 129*2/3 plays the part of 682 = 1023*2/3. */
double
cbrtl (double x)
{
  union {
    double t;
    unsigned char b[5];
  } ut, ux;
  unsigned char sign;
  unsigned int w;
  double r, s, d;
  double lt;

  ux.t = x;
  sign = ux.b[1] & 0x80;		/* sign= sign(x) */
  if (ux.b[0] == 0)
    return x;				/* cbrt(0) is itself */
  ux.b[1] &= 0x7f;			/* |x| */

  /* rough cbrt to 5 bits */
  w = ((unsigned int) ux.b[0] << 7) | ux.b[1];
  w = w / 3 + 11004;			/* 11004 = (86-0.03306235651)*128 */
  ut.b[0] = w >> 7;
  ut.b[1] = w & 0x7f;
  ut.b[2] = 0;
  ut.b[3] = 0;
  ut.b[4] = 0;

  /* new cbrt to 23 bits */
  r=ut.t*ut.t/ux.t;
  s=C+r*ut.t;
  ut.t*=G+F/(s+E+D/s);

  /* chopped to 15 bits and made larger than cbrt(x) */
  ut.b[3] = 0;
  ut.b[4] = 0;
  if (++ut.b[2] == 0 && ++ut.b[1] == 0x80)
    {
      ut.b[1] = 0;
      ut.b[0]++;
    }

  /* one step newton iteration */
  s=ut.t*ut.t;
  r=ux.t/s;
  d=ut.t+ut.t;
  r=(r-ut.t)/(d+r);
  ut.t=ut.t+ut.t*r;

  /* restore the sign bit */
  ut.b[1] |= sign;
  lt = ut.t;
  lt -= (lt - (x/(lt*lt))) * 0.333333333333333333333;
  return lt;
}
#else
double
cbrtl (double x)
{
  long hx;
  double r,s,w;
  double lt;
  unsigned sign;
  typedef unsigned unsigned32 __attribute__((mode(SI)));
  union {
    double t;
    unsigned32 pt[2];
  } ut, ux;
  int n0;

  ut.t = 1.0;
  n0 = (ut.pt[0] == 0);

  ut.t = 0.0;
  ux.t = x;

  hx = ux.pt[n0];			/* high word of x */
  sign=hx&0x80000000;			/* sign= sign(x) */
  hx  ^=sign;
  if(hx>=0x7ff00000) return(x+x);	/* cbrt(NaN,INF) is itself */
  if((hx| ux.pt[1-n0])==0)
    return(ux.t);			/* cbrt(0) is itself */

  ux.pt[n0] = hx;
  /* rough cbrt to 5 bits */
  if(hx<0x00100000)			/* subnormal number */
    {ut.pt[n0]=0x43500000;		/* set t= 2**54 */
     ut.t*=x; ut.pt[n0]=ut.pt[n0]/3+B2;
   }
  else
    ut.pt[n0]=hx/3+B1;

  /* new cbrt to 23 bits, may be implemented in single precision */
  r=ut.t*ut.t/ux.t;
  s=C+r*ut.t;
  ut.t*=G+F/(s+E+D/s);

  /* chopped to 20 bits and make it larger than cbrt(x) */
  ut.pt[1-n0]=0; ut.pt[n0]+=0x00000001;

  /* one step newton iteration to 53 bits with error less than 0.667 ulps */
  s=ut.t*ut.t;				/* t*t is exact */
  r=ux.t/s;
  w=ut.t+ut.t;
  r=(r-ut.t)/(w+r);			/* r-s is exact */
  ut.t=ut.t+ut.t*r;

  /* restore the sign bit */
  ut.pt[n0] |= sign;

  lt = ut.t;
  lt -= (lt - (x/(lt*lt))) * 0.333333333333333333333;
  return lt;
}
#endif /* __cc45__ */

int
main (void)
{
  if ((int) (cbrtl (27.0) + 0.5) != 3)
    abort ();

  exit (0);
}
#else
int main (void) { exit (0); }
#endif
