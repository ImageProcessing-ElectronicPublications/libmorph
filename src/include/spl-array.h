/* spl-array.h: Spline interpolation support routines for matrices
//
// Written and Copyright (C) 1993-1999 by Michael J. Gourlay
//
// NO WARRANTEES, EXPRESS OR IMPLIED.
*/

#ifndef _SPL_ARRAY_H__INCLUDED_
#define _SPL_ARRAY_H__INCLUDED_

#include "spline.h"

int hermite3_array2(const double *kx, const double *ky, long nk,
                double sx_start,double sx_step, double *sy, long ns,
                int howto_extend/* 0-> constant 1-> linear */
               );

extern int hermite3_array (const double *kx, const double *ky, long nk, double *sx, double *sy, long ns);

#endif
