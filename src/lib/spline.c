/* spline.c: cubic spline interpolation
// Spring 1993
//

   Written and Copyright (C) 1993-1999 by Michael J. Gourlay

This file is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This file is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with software; see the file LICENSE.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "spline.h"
#include "my_malloc.h"

/* --------------------------------------------------------------- */

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define ABS(a)   ((a)>=0?(a):(-(a)))

/* --------------------------------------------------------------- */

/* NAME
//   spline3_setup: set parameters for natural cubic spline
//
// ARGUMENTS
//   x (in): knot abcissas
//   y (in): knot ordinates
//   n (in): number of knots
//   c (out): spline parameters
//   h (out): intervals: h[i] = x[i+1] - x[i]
*/
void spline3_setup(const REAL *x, const REAL *y, long n, REAL *c, REAL *h)
{
    REAL *u, *v;
    long i;

    /* only need u and v to index from 1 to n-1 */
    u=MY_CALLOC(n, REAL);
    v=MY_CALLOC(n, REAL);

    for(i=0; i<n; i++)
    {
        h[i]=x[i+1]-x[i];
    }

    for(i=1; i<n; i++)
    {
        v[i]=3.0/h[i]*(y[i+1]-y[i]) - 3.0/h[i-1]*(y[i]-y[i-1]);
        u[i]=2.0*(h[i]+h[i-1]);
    }

    c[0]=c[n]=0;
    for(i=n-1; i>0; i--)
    {
        c[i]=(v[i]-h[i]*c[i+1])/u[i];
    }

    FREE(u);
    FREE(v);
}

/* --------------------------------------------------------------- */
/* NAME
//   spline3_eval -- evaluate the natural cubic spline
//
// ARGUMENTS
//   w (in):  argument abcissa: abcissa-value at which spline is evaluated
//   x (in):  array of knot abcissa values, in increasing order
//   y (in):  array of knot ordinate values
//   n (in):  number of knots
//   c (in):  array of parameters from spline3_setup
//   h (in):  array of intervals between x's
//   s1 (out): spline first derivative
//   s2 (out): spline second derivative
//
// DESCRIPTION
//   Evaluate a spline interpolant at the given abcissa "w".
//   if s1==NULL then s1 is not evaluated
//   if s2==NULL then s2 is not evaluated
//
// RETURN VALUE
//   spline interpolant value at "w".
*/
REAL spline3_eval(REAL w, const REAL *x, const REAL *y, long n,
             const REAL *c, const REAL *h, REAL *s1, REAL *s2)
{
    REAL diff=0.0;
    REAL b, d;
    long i;

    /* find interval of spline to evaluate */
    for(i=n-1; (i>=0) && ((diff=(w-x[i]))<0.0); i--)
        ;

    /* calculate other spline parameters */
    /* here a=y[i] so it is not explicitly named a */
    b = (y[i+1]-y[i])/h[i] - h[i]/3.0*(2.0*c[i] + c[i+1]);
    d = (c[i+1]-c[i])/h[i];

    /* evaluate derivatives of spline */
    if(s1!=NULL) *s1 = b + diff*(2.0*c[i] + 3.0*d*diff);
    if(s2!=NULL) *s2 = 2.0*(c[i] + 3.0*d*diff);

    /* return spline value */
    return (y[i] + diff*(b + diff*(c[i] + diff*d)));
}

/* --------------------------------------------------------------- */
/* NAME
//   d_parabola -- returns the derivative of a parabola fit through 3 points
//
// To find this formula:
//   A parabola is
//     y = a x^2 + b x + c.  (eqn 1)
//   Write this for 3 points, (x0,y0) (x1,y1), (x2,y2):
//     y0 = a x0^2 + b x0 + c  (eqn 2 i)
//     y1 = a x1^2 + b x1 + c  (eqn 2 ii)
//     y2 = a x2^2 + b x2 + c  (eqn 2 iii)
//   Solve this system for the constants a, b, c.
//   Substitute these a, b, c back into (eqn 1).  Take the derivative of
//   (eqn 1).
*/
REAL d_parabola(REAL x, REAL xp0, REAL xp1, REAL xp2,
           REAL yp0, REAL yp1, REAL yp2)
{
    REAL dP=(  xp0*(yp1-yp2)*(2.0*x - xp0)
               + xp1*(yp2-yp0)*(2.0*x - xp1)
               + xp2*(yp0-yp1)*(2.0*x - xp2)) / ((xp0-xp1)*(xp0-xp2)*(xp2-xp1));

    return dP;
}

/* --------------------------------------------------------------- */
/* NAME
//   hermite3_interp : cubic hermite interpolation
//
//
// ARGUMENTS
//   w (in): evaluation abcissa
//     w should be in the range x[0] <= w <= x[n-1]
//
//   x (in): abcissas at knots
//     abcissas should be ordered such that x[0] < x[1] < ... < x[n-1]
//
//   y (in): ordinates at knots
//
//   d (in/out): derivatives at knots
//
//   n: (in): number of knots
//
//   f: (in): derivative function (or NULL if not available)
//
//   flags (in): bit field:
//     (flags&1): compute_deriv:
//                0 => use d as input, 1 => compute d
//                NOTE: if compute_deriv == 1 and f == NULL then the
//                      derivatives are estimated using a parabola fit.
//     (flags&2): periodic:
//                0 => non-periodic domain, 1 => periodic domain
//                NOTE:  for periodic domain, the abcissa of the
//                  point outside of the explicitly provided domain
//                  is estimated by using the distance between the
//                  outer 2 points to find the distance to the next
//                  point.
//   h1 (out): first derivative of spline, NULL=>ignore
//
//   h2 (out): second derivative of spline, NULL=>ignore
//
//
// DESCRIPTION
//   This interpolant is cubic (i.e. is a third order polynomial) fit
//   through two knots where the values of the function and the values of
//   the first derivative of the function are known at the knots.
//   (Stricly speaking, the values of the derivatives do not need to be
//   analytically known, since some estimate of the derivative can be
//   used.)
//
//
// RETURN VALUE
//   Evaluation of interpolation
//
//
// NOTES
//   The estimated derivatives can be pretty lousy.
//
//   To derive this formula:
//     Call the interpolant H(x).  Name the values
//       H(x_i)     = y_i        H'(x_i)     = d_i
//       H(x_(i+1)) = y_(i+1)    H'(x_(i+1)) = d_(i+1)
//     The interpolant has the form
//       H(x) = y_i + h_i d_i + h_i^2 A + h_i^2 B (x - x_(i+1))
//     where
//       h_i = x_(i+1) - x_i
//     The derivative therefore has the form
//       H'(x) = d_i + 2 A h_i + B (2 h_i (x - x_(i+1)) + h_i^2)
//     Now substitute in the values at x_(i+1) into H and H', and set them
//     equal to y_(i+1) and d_(i+1) respectively to get A and B.
//
//     The interpolant is a proper function, where x is the abcissa and
//     y is the ordinate.  However, if data is provided such that
//     the interpolant is not a function such that each abcissa
//     has exactly one ordinate (i.e. where some subset of x values
//     overlap), then the interpolant will give incorrect results.
//     If this is the case then the data you are trying to interpolate
//     can not be represented by a proper function, so you will have
//     to parameterize the data with some third parameter variable
//     (usually refered to as t), and interpolate x and y with respect
//     to that third variable.
*/
REAL hermite3_interp(REAL w, const REAL *x, const REAL *y, REAL *d, long n,
                REAL (*f)(REAL), int flags, REAL *h1, REAL *h2)
{
    double A, B;
    double h, h_2;
    double diff=0.0;
    double H;
    long si;
    int compute_deriv = (flags&1);
    int periodic = (flags&2);

#if (VERBOSE >= 3)
    printf("hermite3_interp: at %12g\n", w);
    for(si=0; si < n; si++)
    {
        printf("hermite3_interp: [%4li]  x = % 12g  y = % 12g\n", si, x[si], y[si]);
    }
#endif

    /* Find interval of spline to evaluate
    // I.e. find the index such that the evaluation abcissa, w, is to
    // the right of the abcissa associated with that index.  Then, x is
    // between x[si] and x[si+1].
    */
    /* MJG 14jul95 -- do not start at si=n-1.  See "h =" line below.
    // MJG 18jul94 -- was reading beyond bounds at last knot. (maybe)
    */
    for(si=n-2; (si>=0) && ((diff=(w-x[si])) < 0.0); si--)
        ;

    /* We are sitting at a knot so no interpolation needed */
    if(0.0 == diff)
    {
#if (VERBOSE >= 3)
        printf("hermite3_interp: at knot %li\n", si);
#endif
        return y[si];
    }

    /* h is the interval between knots */
    h   = x[si+1] - x[si];
    h_2 = h*h;

#if (VERBOSE >= 3)
    printf("hermite3_interp: si = %li    diff = %12g\n", si, diff);
    printf("hermite3_interp: h = %12g    h^2 = %12g\n", h, h_2);
#endif

    /* either the derivatives were provided or must be found */
    if(compute_deriv)
    {
        /* must calculate derivatives */
        if(f != NULL)
        {
            /* calculate the derivative */
            d[si]=(*f)(x[si]);
            d[si+1]=(*f)(x[si+1]);

        }
        else
        {
            /* approximate derivative using parabola fit */
            if(0 == si)
            {
                /* at first knot */
                d[si+1] = d_parabola(x[si+1], x[si], x[si+1], x[si+2],
                                     y[si], y[si+1], y[si+2]);
                if(!periodic)
                {
                    d[si] = d_parabola(x[si], x[si], x[si+1], x[si+2],
                                       y[si], y[si+1], y[si+2]);
                }
                else     /* periodic */
                {
                    /* estimate left-end abcissa */
                    REAL xn = 2.0 * x[si] - x[si+1];
                    d[si] = d_parabola(x[si], xn,     x[si], x[si+1],
                                       y[n-1], y[si], y[si+1]);
                }

            }
            else if(si >= (n-2))
            {
                /* at last or 2nd to last knot */
                d[si] = d_parabola(x[si], x[si-1], x[si], x[si+1],
                                   y[si-1], y[si], y[si+1]);
                if(!periodic)
                {
                    d[si+1] = d_parabola(x[si+1], x[si-1], x[si], x[si+1],
                                         y[si-1], y[si], y[si+1]);
                }
                else     /* periodic */
                {
                    /* estimate right-end abcissa */
                    REAL xn = 2.0 * x[si+1] - x[si];
                    d[si+1] = d_parabola(x[si+1], x[si], x[si+1], xn,
                                         y[si], y[si+1], y[0]);
                }

            }
            else
            {
                /* between first and 2nd to last knot */
                d[si] = d_parabola(x[si], x[si-1], x[si], x[si+1],
                                   y[si-1], y[si], y[si+1]);
                d[si+1] = d_parabola(x[si+1], x[si], x[si+1], x[si+2],
                                     y[si], y[si+1], y[si+2]);
            }
        }
    }

    /* calculate interpolant parameters */
    A = (y[si+1] - y[si] - h*d[si])/h_2;
    B = (d[si+1] - d[si] - 2.0*h*A)/h_2;

    /* evaluate spline derivatives */
    if(h1!=NULL) *h1 = d[si] + diff*(2.0*A + B*(diff + 2.0*(w-x[si+1])));
    if(h2!=NULL) *h2 = 2.0*A + 2.0*B*(2.0*diff + (w-x[si+1]));

    /* return the spline evaluation */
    H  =  y[si] + diff*(d[si] + diff*(A + (w-x[si+1])*B));

    return H;
}
