/* spline.h: Interpolation
//
// Written and Copyright (C) 1993-1999 by Michael J. Gourlay
//
// NO WARRANTEES, EXPRESS OR IMPLIED.
//
// The macro "REAL" must be defined from the compile command line for
// this module to compile.  Do NOT !!!! NOT!!!! define REAL inside of
// this or ANY other C module.  It will lead to all kinds of headaches
// down the road when multiple files define REAL in conflicting ways,
// especially if you hide the #define REAL inside of #ifndef REAL
// blocks in order to try to be slick, because one module will expect
// REAL to be one thing and another module will not really care but
// will define REAL in another way, and you WILL forget that you have
// defined REAL in more than one place.
*/

#ifndef _SPLINE_H__INCLUDED_
#define _SPLINE_H__INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

    void spline3_setup(const REAL *x, const REAL *y, long n, REAL *c, REAL *h);
    REAL spline3_eval(REAL w, const REAL *x, const REAL *y, long n, const REAL *c, const REAL *h, REAL *s1, REAL *s2);
    REAL d_parabola(REAL x, REAL xp0, REAL xp1, REAL xp2, REAL yp0, REAL yp1, REAL yp2);
    REAL hermite3_interp(REAL w, const REAL *x, const REAL *y, REAL *d, long n, REAL (*f)(REAL), int flags, REAL *h1, REAL *h2);


#ifdef __cplusplus
}
#endif

#endif
