/* warp.h : Digital Image Warping algorithms header
//
// Written and Copyright (C) 1994-1999 by Michael J. Gourlay
//
// Provided as is.  No warrantees, express or implied.
*/

#ifndef _WARP__INCLUDED_
#define _WARP__INCLUDED_
#include "braindead_msvc.h"

/* this is the C type of a pixel */
#define PIXEL_TYPE unsigned char
#define PIXEL_MIN 0
#define PIXEL_MAX 255

/* choose  warp engine   */
#define warp_image warp_image_inv_new

void warp_image_inv_old
(const PIXEL_TYPE *in, PIXEL_TYPE *out,
 int img_width, int img_height,
 const double *xs, const double *ys, const double *xd, const double *yd,
 int mesh_width, int mesh_height);


#endif
