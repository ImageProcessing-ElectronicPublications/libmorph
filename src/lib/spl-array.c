/* spl-array.c: Spline interpolation support routines for matrices
//
// Written and Copyright (C) 1993-1999 by Michael J. Gourlay
//
// PROVIDED AS IS.  NO WARRANTEES, EXPRESS OR IMPLIED.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "my_malloc.h"
#include "spline.h"
#include "spl-array.h"
#include "braindead_msvc.h"

/* --------------------------------------------------------------- */

#define NEAR(x1, x2) (((x2)!=0.0) && (((x1)/(x2)) >= 0.999) && (((x1)/(x2))<1.001))

#ifndef FALSE
#define FALSE 0
#endif

/* --------------------------------------------------------------- */

/* derivative_hack: compute 1st derivative of x,y data (len entries)
//
// Written and Copyright (C) 1994-1997 by Michael J. Gourlay
//
// PROVIDED AS IS.  NO WARRANTEES, EXPRESS OR IMPLIED.
//
// Mathematically, it's a hack to prevent overshooting knots, but
// maintain smoothness and it works more intuitively.  Besides, we all
// know that mathematicians are too worried about rigor, and the
// physicists end up creating the right math. -- MJG
*/

/* avoid division by zero */
static inline double NONZERO(double A)
{
    return  (A>0.01 ? A :
             ( A >0 ? 0.01 :
               ( A > -0.01 ? -0.01 : A)));
}

static void derivative_hack(const double *x, const double *y, double *yd, int len)
{
    int indx;

    yd[0] = (y[1]-y[0])/NONZERO(x[1]-x[0]);

    yd[len-1] = (y[len-1]-y[len-2])/NONZERO(x[len-1]-x[len-2]);

    for(indx=1; indx<(len-1); indx++)
    {
        if( ((y[indx-1] >= y[indx]) && (y[indx+1] >= y[indx]))  ||
                ((y[indx-1] <= y[indx]) && (y[indx+1] <= y[indx]))    )
        {
            /* There was a change in the sign of yd so force it zero */
            /* This will prevent the spline from overshooting this knot */
            yd[indx] = 0.0;
        }
        else
        {
            /* Set slope at this knot to slope between two adjacent knots */
            yd[indx] = (y[indx-1]-y[indx+1]) / NONZERO(x[indx-1]-x[indx+1]);
        }
    }
}

/* --------------------------------------------------------------- */

/* hermite3_array : cubic hermite interpolation for an array of points
//
// Uses derivative_hack to find derivatives.
//
//
// kx, ky (in):  arrays of knots
// nk (in):      number of knots
// sx (in):      evaluation points array
// sy (out):     spline values at evaluation pts sx.  Must already be allocated.
// ns (in):      number of evaluation points
*/

int hermite3_array(const double *kx, const double *ky, long nk, double *sx,
               double *sy, long ns)
{
    register long xi;
    double *kyd;
    if((kyd=MY_CALLOC(nk, double))==NULL)
        return 1;

    /* Test bounds. */
    /* As of 18jul94, this test was triggering for cases
    // where the bounds were nearly equal, but slightly out of range, in
    // which case the spline should work anyway, which is why I let it run
    // even if the spline abcissas are out of range.
    */
#ifdef USELESS /* and dangerous , now that border is free */
    if((sx[0] < kx[0]) || (sx[ns-1] > kx[nk-1]))
    {
        if(!NEAR(sx[ns-1], kx[nk-1]))
        {
            fprintf(stderr, "hermite3_array: out of range:\n");
            fprintf(stderr,
                    "hermite3_array: eval=%.20g < knot=%.20g | %.20g>%.20g\n",
                    sx[0], kx[0], sx[ns-1], kx[nk-1]);
        }
    }
#endif

    /* Find array of derivatives */
    derivative_hack(kx, ky, kyd, nk);

    /* Evaluate the spline */
    for(xi=0; xi<ns; xi++)
    {
        if (sx[xi]<kx[0])
            sy[xi]=ky[0];
        else if (sx[xi] > kx[nk-1])
            sy[xi]=ky[nk-1];
        else
            sy[xi]=hermite3_interp(sx[xi], kx, ky, kyd, nk, NULL, FALSE, NULL, NULL);
    }

    /* Free the derivatives array */
    FREE(kyd);
    return 0;
}

int hermite3_array2(const double *kx, const double *ky, long nk,
                double sx_start,double sx_step, double *sy, long ns,
                int howto_extend/* 0-> constant 1-> linear */
               )
{
    register long xi=0;
    double *kyd;
    if((kyd=MY_CALLOC(nk, double))==NULL)
        return 1;

    /* Find array of derivatives */
    derivative_hack(kx, ky, kyd, nk);

    assert(sx_step>=1);

    /* Evaluate the spline */
    for(; sx_start+xi*sx_step <kx[0] && xi<ns; xi++)
        if(howto_extend)
            sy[xi]=ky[0]-kx[0]+sx_start+xi*sx_step;
        else
            sy[xi]=ky[0];

    for (; sx_start+xi*sx_step < kx[nk-1] && xi<ns;    xi++)
        sy[xi]=hermite3_interp(sx_start+sx_step*xi,
                               kx, ky, kyd, nk, NULL, FALSE, NULL, NULL);
    for(; xi<ns; xi++)
        if(howto_extend)
            sy[xi]=ky[nk-1]-kx[nk-1]+sx_start+xi*sx_step;
        else
            sy[xi]=ky[nk-1];

    /* Free the derivatives array */
    FREE(kyd);

    /*   FREE(kx);FREE(ky); */
    return 0;
}

#ifdef BLAH
double *kx, *ky;
ky=MY_CALLOC(nk+2, double);
kx=MY_CALLOC(nk+2, double);
memcpy(kx+1,kx_orig,nk*sizeof(double));
memcpy(ky+1,ky_orig,nk*sizeof(double));
kx[0]=kx_orig[0]-400;
kx[nk+1]=kx_orig[nk-1]+400;
ky[0]=ky_orig[0];
ky[nk+1]=ky_orig[nk-1];
nk+=2;
#endif

int bilinear_array(const double *kx, const double *ky, long nk, double *sx,
               double *sy, long ns)
{
    int xi=0,ki=0;
    double v,d;
    while(xi<ns)
    {
        while(sx[xi]>kx[ki] && ki<nk) ki++;

        if(ki==0)
            v=ky[0];
        else if(ki==nk)
            v=ky[nk-1];
        else
        {
            d=(kx[ki]-kx[ki-1]);
            v= ky[ki-1] * (kx[ki]-sx[xi]) +  ky[ki] * (sx[xi]-kx[ki-1]);
            v/=d;
        }
        sy[xi]=v;
        xi++;
    }
    return 0;
}
