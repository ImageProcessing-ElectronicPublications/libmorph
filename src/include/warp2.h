
#ifndef __LIBMORPH_WARP2_H__
#define __LIBMORPH_WARP2_H__
#include "mesh_t.h"

/* for PIXEL_TYPE */
#include "warp.h"

/*   warp_image: 2-pass mesh-based image warping   */

/* NAME
//   warp_image_versatile
//
// ARGUMENTS
//   src (in)         : input image (1 PIXEL_TYPE per pixel)
//   dst (out)       : output image (1 PIXEL_TYPE per pixel)
//
// for each image,
//   _width (in)  : width dimension, in pixels
//   _height (in) : height dimension, in pixels
//   _channels (in): channels
//   _rowstride: number to add to a ptr PIXELTYPE *, to move down one pixel in the image
//   _xstride:  number to add to a ptr PIXELTYPE *, to move right one pixel in the image
//
//   xs (in)         : source mesh x-coordinate values
//   ys (in)         : source mesh y-coordinate values
//   xd (in)         : destination mesh x-coordinate values
//   yd (in)         : destination mesh y-coordinate values
//   mesh_width (in) : x-dimension of meshes
//   mesh_height (in): y-dimension of meshes
//
//
// AUTHOR, NOTES: see warp.c
*/

void warp_image_versatile
(
    const PIXEL_TYPE *src,
    int s_width, int s_height, int s_channels, int s_rowstride,int s_xstride,
    PIXEL_TYPE *dst,
    int d_width, int d_height, int d_channels, int d_rowstride,int d_xstride,
    const double *xs, const double *ys,  const double *xd, const double *yd,
    int mesh_width, int mesh_height);

void warp_image_inv_new(const PIXEL_TYPE *in, PIXEL_TYPE *out, int img_width, int img_height, const double *xs, const double *ys, const double *xd, const double *yd, int mesh_width, int mesh_height);

void  warp_image_a_m
(const PIXEL_TYPE *src,
 int s_width, int s_height, int s_channels, int s_rowstride,int s_xstride,
 PIXEL_TYPE *dst,
 int d_width, int d_height, int d_channels, int d_rowstride,int d_xstride,
 MeshT *srcmesh, MeshT *dstmesh);

#endif // __LIBMORPH_WARP2__
