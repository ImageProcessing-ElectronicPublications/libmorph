/* RgbaImage.h : image handling routines
//

   Written and Copyright (C) 1994-1999 by Michael J. Gourlay

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

/* SWIG: Simplified Wrapper Interface Generator */
#ifdef SWIG
%module RgbaImage
%
{
#include "rgbaimage.h"
#include "mesh.h"
    %
}
#endif


#ifndef _RGBA_IMAGE_H__INCLUDED_
#define _RGBA_IMAGE_H__INCLUDED_




/* BITS_PER_CHANNEL is the number of bits per channel per pixel that
// an rgba_image uses to store image data.
// This should be the same as the size of the storage class used for
// rgba_image channels, i.e. unsigned char.
*/
#define BITS_PER_CHANNEL 8




/* RGBA_IMAGE_MAXVAL is the maximum value a channel can have for a
// single pixel.
*/
#define RGBA_IMAGE_MAXVAL ((1<<BITS_PER_CHANNEL) - 1)

/* RGBA_IMAGE_OPAQUE is the same as RGBA_IMAGE_MAXVAL, except that
// it's for the alpha (opacity) channel, and this is a reminder that zero
// is transparent and the MAXVAL is opaque.
*/
#define RGBA_IMAGE_OPAQUE RGBA_IMAGE_MAXVAL

typedef struct rgba_image_
{
    int           nrows;          /* number of rows in (i.e. height of) image */
    int           ncols;          /* number of columns in (i.e. width of) image */
    int           compressed;     /* whether image is compressed */
    int           pixel_size;     /* number of bits per pixel */
    int           color_mapped;   /* whether image is color mapped */
    int           type;           /* image type token */
    unsigned char *ri;            /* red channel image data */
    unsigned char *gi;            /* green channel image data */
    unsigned char *bi;            /* blue channel image data */
    unsigned char *ai;            /* alpha (opacity) channel image data */
} RgbaImageT;

#ifdef SWIG
#include <tcl.h>
#include <tk.h>
%addmethods RgbaImageT
{
    RgbaImageT(void)
    {
        return rgbaImageNew();
    }

    ~RgbaImageT(void)
    {
        rgbaImageDelete(self);
    }

    int alloc(int nx, int ny)
    {
        return rgbaImageAlloc(self, nx, ny);
    }

    void free()
    {
        rgbaImageFree(self);
    }

    int dissolve(const RgbaImageT *siP, const RgbaImageT *diP, float dissolve)
    {
        return rgbaImageDissolve(self, siP, diP, dissolve);
    }

    int read(const char *filename)
    {
        return rgbaImageRead(self, filename);
    }

    int write(const char *filename)
    {
        return rgbaImageWrite(filename, self, NULL, 0.0);
    }

    int reset(int type)
    {
        return rgbaImageTestCreate(self, type);
    }

    void toPhoto(char * photo_tag)
    {
        /* Convert photo tag into photo handle */
        Tk_PhotoHandle photoH = Tk_FindPhoto(photo_tag);
        rgbaImageTkPhotoConvert(self, photoH);
    }

    int warp(const RgbaImageT *img_orig, const MeshT*mesh_src,
    const MeshT *mesh_dst, float tween_param)
    {
        return rgbaImageWarp(img_orig, self, mesh_src, mesh_dst, tween_param);
    }

};
#endif

void rgbaImageInit(RgbaImageT *self) ;

RgbaImageT * rgbaImageNew(void) ;

void rgbaImageDelete(RgbaImageT *self);

void rgbaImageFree(RgbaImageT *self);

int rgbaImageAlloc(RgbaImageT *self, const int nx, const int ny);

int rgbaImageDissolve(RgbaImageT *self, const RgbaImageT *siP, const RgbaImageT *diP, float dissolve);

int rgbaImageRead(RgbaImageT *self, const char *filename);

int rgbaImageWrite(const char *filename, const RgbaImageT *siP, const RgbaImageT *diP, float dissolve);

#ifdef NEED_GIMP
#include <libgimp/gimp.h>
int rgbaImageUnGIMP (RgbaImageT *self, GDrawable *d);

int rgbaImageGIMP (GDrawable *d, const RgbaImageT *siP, const RgbaImageT *diP,
                   float dissolve);
#endif

int rgbaImageTestCreate(RgbaImageT *self, int type);

#ifdef RGBA_MESH_WARP
#include "mesh.h"
int rgbaImageWarp(const RgbaImageT *img_orig, RgbaImageT *img_warp, const MeshT *mesh_src, const MeshT *mesh_dst, float tween_param);
#endif

#ifdef RGBA_TK
#include <tcl.h>
#include <tk.h>
void rgbaImageTkPhotoConvert(RgbaImageT *self, Tk_PhotoHandle photoH);
#endif

#endif
