/* warp.c : Digital Image Warping algorithms
//
// See George Wolberg's "Digital Image Warping"
// IEEE Computer Society Press order number 1944
// ISBN 0-8186-8944-7
//
// Copyrights might be held by various authors.  See individual routines.
//

   Written and Copyright (C) 1994-1999 by Michael J. Gourlay
   Written and Copyright (C) 2003      by A Mennucc

This file is part of Xmorph.

Xmorph is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

Xmorph is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Xmorph; see the file LICENSE.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

*/

#include <stdio.h>
#include <stdlib.h>

#include "my_malloc.h"
#include "spline.h"
#include "spl-array.h"
#include "mesh.h"

#include "warp.h"
#include "warp2.h"

#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define MIN(x,y) ((x)<(y) ? (x) : (y))


#ifndef g_assert
#include "assert.h"
#define g_assert  assert
#endif

#include "resample.h"


/* NAME
//   warp_image: 2-pass mesh-based image warping
//
//
// ARGUMENTS: see warp2.h
//
// NOTES
//
//  A Mennucc: I have had a striking idea!!!
//    I have inverted the roles of src & dst!
//    so the resample_array becomes resample_array_inv
//    which pulls pixels into dst instead of pushing from src.
//
//    Improvements: mesh lines may now fold over!
//    and the border of the mesh is now free!
//    you can warp images of different sizes, and with colors!
//    the code is smaller, faster, cleaner!
//    (final score: mathematician 1, physicists 1
//        :-) see spl-array.c)
//
// AUTHOR
//   This code was originally written by George Wolberg, based on Smythe90.
//   Modifications and many bug fixes by Michael J. Gourlay and other
//   authors as noted in the code comments.
//
//
// SEE ALSO
//   See Douglas B. Smythe "A Two-Pass Mesh Warping Algorithm for
//   Object Transformation and Image Interpolation", ILM Technical Memo
//   #1030, Computer Graphics Department, Lucasfilm Ltd., 1990
//
//
*/

void warp_image_versatile
(const PIXEL_TYPE *src,
 int s_width, int s_height, int s_channels, int s_rowstride,int s_xstride,
 PIXEL_TYPE *dst,
 int d_width, int d_height, int d_channels, int d_rowstride,int d_xstride,
 const double *xs, const double *ys,  const double *xd, const double *yd,
 int mesh_width, int mesh_height)
{
    /* "const type *" means "pointer to a constant array of type"
    // not "constant pointer to type"
    */

    int ai    = MAX(MAX(s_height, s_width) ,
                    MAX(d_height, d_width) )    + 1;
    double *ind;
    int i;

    int          xi, yi;

    /* specs of intermidiate image */
    PIXEL_TYPE        *tmp;
    int c,t_channels = MIN(s_channels, d_channels);
    int t_rowstride=d_width*t_channels;

    const double *x1, *y1, *x2, *y2;
    double       *xrow1, *yrow1, *xrow2, *yrow2;
    double       *map1, *map2;
    double       *ts, *ti, *td;



    ind  =  MY_CALLOC(ai, double);
    for(i=0; i<ai; i++)     ind[i] = i;

    /* allocate memory for buffers:
    //
    // indx stores indices used to sample splines
    // xrow1, xrow2, yrow2, yrow2 store column data in row order for spline
    // map1, map2 store mapping functions computed in row order in spline
    //
    // Could use alloca instead to avoid the free's at the end of this
    // routine.  alloca might also be faster.  In fact, the original code
    // might have used alloca.  I do not remember why I changed it.
    */

    xrow1 =  MY_CALLOC(ai, double);
    yrow1 =  MY_CALLOC(ai, double);
    xrow2 =  MY_CALLOC(ai, double);
    yrow2 =  MY_CALLOC(ai, double);
    map1  =  MY_CALLOC(ai, double);
    map2  =  MY_CALLOC(ai, double);


    /* First pass (phase one): create tables ts and ti for x-intercepts
    // of vertical splines in SRC and TMP.
    //
    // NOTE: the tmp image is  s_height x d_width
    //
    // tables have mesh_width columns of height _height
    */
    ts = MY_CALLOC(mesh_width*s_height, double);
    ti = MY_CALLOC(mesh_width*s_height, double);

    for(xi=0; xi<mesh_width; xi++)   /* visit each vertical spline */
    {
        /* store columns as rows for spline */
        for(yi=0; yi<mesh_height; yi++)
        {
            xrow1[yi] = xs[yi*mesh_width+xi];
            xrow2[yi] = xd[yi*mesh_width+xi];
            yrow1[yi] = ys[yi*mesh_width+xi];
            yrow2[yi] = yd[yi*mesh_width+xi];
        }

        /* scan convert vertical splines of S and I */
        hermite3_array2(yrow1, xrow1, mesh_height, 0,1, map1, s_height,0);
        /* bug reported 30may96 by Mike Hoefelein: */
        /* hermite3_array(yrow2, xrow2, mesh_height, 0,1, map2, _height); */
        hermite3_array2(yrow1, xrow2, mesh_height, 0,1, map2, s_height,0);

        /* store resampled rows back into columns */
        for(yi=0; yi<s_height; yi++)
        {
            ts[yi*mesh_width+xi] = map1[yi];
            ti[yi*mesh_width+xi] = map2[yi];
        }
    }

    /* First pass (phase two): warp x using ts and ti.
    // tmp holds intermediate image.
    */
    if((tmp = MY_CALLOC( s_height * d_width * t_channels,PIXEL_TYPE))==NULL)
    {
        fprintf(stderr, "warp_image: Bad Alloc: tmp\n");
        return;
    }

    for(yi=0; yi < s_height; yi++)   /* visit each row */
    {
        /* fit spline to x-intercepts; resample over all columns */
        x1 = &ts[yi*mesh_width];
        x2 = &ti[yi*mesh_width];
        hermite3_array2(x2, x1, mesh_width, 0,1, map1, d_width,1);

        /* resample source row based on map1 */
        for (c=0; c<t_channels; c++)
            resample_array_inv(map1,
                               src+yi*s_rowstride+c, s_width, s_xstride,
                               tmp+yi*t_rowstride+c, d_width, t_channels);
    }

    /* free buffers */
    FREE(ts);
    FREE(ti);

    /* Second pass (phase one): create tables ti and td for y-intercepts
    // of horiz splines in TMP and DST.
    // Tables have mesh_height rows of width d_width
    */
    ti = MY_CALLOC(mesh_height*d_width, double);
    td = MY_CALLOC(mesh_height*d_width, double);


    for(yi=0; yi < mesh_height; yi++)
    {
        /* scan convert horizontal splines of TMP and DST */

#if 0
        x1 = &xs[yi*mesh_width];
#endif

        y1 = &ys[yi*mesh_width];
        x2 = &xd[yi*mesh_width];
        y2 = &yd[yi*mesh_width];

        /* The following line is correct: (x2, y1, ...) */
        hermite3_array2(x2, y1, mesh_width, 0,1, &ti[yi*d_width], d_width,0);
        hermite3_array2(x2, y2, mesh_width, 0,1, &td[yi*d_width], d_width,0);
    }

    /* Second pass (phase two): warp y using ti and td */

    for(xi=0; xi < d_width; xi++)
    {
        /* store columns as row for hermite3_array */
        for(yi=0; yi<mesh_height; yi++)
        {
            xrow1[yi] = ti[yi*d_width+xi];
            yrow1[yi] = td[yi*d_width+xi];
        }

        /* fit spline to y-intercepts:  resample over all rows */
        hermite3_array2(yrow1, xrow1, mesh_height, 0,1, map1, d_height,1);

        /* resample intermediate image column based on map */
        for (c=0; c<t_channels; c++)
            resample_array_inv(map1,
                               tmp+xi*t_channels+c, s_height, t_rowstride,
                               dst+xi*d_xstride +c, d_height, d_rowstride);
    }

    FREE(tmp);
    FREE(ti);
    FREE(td);
    FREE(ind);
    FREE(xrow1);
    FREE(yrow1);
    FREE(xrow2);
    FREE(yrow2);
    FREE(map1);
    FREE(map2);

}


/**************************************************** USEFUL INTERFACES  *****/

void warp_image_inv_new(const PIXEL_TYPE *in, PIXEL_TYPE *out,
                   int img_width, int img_height,
                   const double *xs, const double *ys,
                   const double *xd, const double *yd,
                   int mesh_width, int mesh_height)
{
    warp_image_versatile
    (in,
     img_width,  img_height,1,img_width,1,
     out,
     img_width,  img_height,1,img_width,1,
     xs,  ys, xd,  yd,
     mesh_width,  mesh_height);
}


void warp_image_a_m
(const PIXEL_TYPE *src,
 int s_width, int s_height, int s_channels, int s_rowstride,int s_xstride,
 PIXEL_TYPE *dst,
 int d_width, int d_height, int d_channels, int d_rowstride,int d_xstride,
 MeshT *srcmesh, MeshT *dstmesh)
{
    g_assert( srcmesh->nx == dstmesh->nx && srcmesh->ny == dstmesh->ny);
    warp_image_versatile
    (src,
     s_width,  s_height,  s_channels,  s_rowstride, s_xstride,
     dst,
     d_width,  d_height,  d_channels,  d_rowstride, d_xstride,
     srcmesh->x, srcmesh->y,
     dstmesh->x, dstmesh->y,
     srcmesh->nx,srcmesh->ny);
}
