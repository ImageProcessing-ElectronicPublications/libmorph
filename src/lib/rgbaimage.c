/* RgbaImage.c : RGBA image handling routines
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

#ifdef GIMP
# define NEED_GIMP 1
# include <libgimp/gimp.h>
# include <string.h>
/* Number of steps in the progress meter. */
extern int prog_step;
extern int prog_nsteps;
#else
# define MAX(x,y) ((x)>(y) ? (x) : (y))
# define MIN(x,y) ((x)<(y) ? (x) : (y))
#endif

#include <stdio.h>
#include <stdlib.h>

#include "my_malloc.h"
#include "tga.h"
#include "rgbaimage.h"

/* Dissolve macros to speed up the GIMP saving function. */
#define GRAY_DISSOLVE(si,di,pel,t) \
  (((1.0 - t) * (si->ri[pel] + si->gi[pel] + si->bi[pel]) \
    + (t * (di->ri[pel] + di->gi[pel] + di->bi[pel]))) / 3)

#define RED_DISSOLVE(si,di,pel,t) \
  (((1.0 - t) * si->ri[pel]) + (t * di->ri[pel]))

#define GREEN_DISSOLVE(si,di,pel,t) \
  (((1.0 - t) * si->gi[pel]) + (t * di->gi[pel]))

#define BLUE_DISSOLVE(si,di,pel,t) \
  (((1.0 - t) * si->bi[pel]) + (t * di->bi[pel]))

#define ALPHA_DISSOLVE(si,di,pel,t) \
  (((1.0 - t) * si->ai[pel]) + (t * di->ai[pel]))

/* NAME
//   rgbaImageInit: initialize members of an RgbaImage
*/
void rgbaImageInit(RgbaImageT *self)
{
    self->nrows = 0;
    self->ncols = 0;
    self->compressed = 0;
    self->pixel_size = 0;
    self->color_mapped = 0;
    self->type = 0;
    self->ri = NULL;
    self->gi = NULL;
    self->bi = NULL;
    self->ai = NULL;
}

/* NAME
//   rgbaImageNew: Allocate and initialize an RgbaImageT instance
//
//
// DESCRIPTION
//   An RgbaImage is an image that stores red, green, blue, and opacity
//   information as arrays of bytes, where each byte stores a pixel
//   value.  The key feature of the storage scheme is that each channel
//   is stored as a contiguous array, rather than storing the image
//   channels data interleaved.  The reason why storing image data as
//   contiguous channels is that some image processing routines are
//   designed to operate on a single channel only, or on monochrome
//   images.  By storing full-color images as contiguous arrays of single
//   channels, we can use one-channel image processing routines.  In
//   particular, the warp_image routine expects single channels and that
//   is what we want to use.
*/
RgbaImageT * rgbaImageNew(void)
{
    RgbaImageT *rgba_image = MY_CALLOC(1, RgbaImageT);

    if(NULL == rgba_image)
    {
        return NULL;
    }

    rgbaImageInit(rgba_image);

    return rgba_image;
}

/* NAME
//   rgbaImageDelete: Delete an RgbaImageT instance
//
//
// NOTES
//   rgbaImageDelete does NOT free the channel arrays.  The channel
//   arrays should be freed before calling rgbaImageDelete.
//
//
// SEE ALSO
//   rgbaImageFree, rgbaImageNew, rgbaImageAlloc
*/
void rgbaImageDelete(RgbaImageT *self)
{
#if DEBUG >= 1
    printf("rgbaImageDelete: %p\n", self);
#endif

    FREE(self);
}

/* NAME
//   rgbaImageFree: free memory of the RgbaImage channels.
//
//
// NOTES
//   Memory for the RgbaImageT instance is not freed here.
//
//   The image channel memory is assumed to be contiguous, so that only
//   the "ri" channel is actually called with "free".
//
//
// SEE ALSO
//   rgbaImageAlloc, rgbaImageDelete
*/
void rgbaImageFree(RgbaImageT *self)
{
    if(self->ri != NULL)
    {
        FREE(self->ri);
        self->ri = NULL;
        self->gi = NULL;
        self->bi = NULL;
        self->ai = NULL;
        self->nrows = 0;
        self->ncols = 0;
    }
}

/* NAME
//   rgbaImageAlloc: allocate memory for the RgbaImage channels
//
//
// ARGUMENTS
//   self (in/out):  pointer to RgbaImage.
//     The ncols,nrows,ri,gi,bi,ai members are set.
//
//   nx (in): image width.  ncols is set to this.
//
//   ny (in): image height.  nrows is set to this.
//
//
// DESCRIPTION
//   Use only one allocation to ensure that the image data is
//   contiguous.  This makes it easier to use other image format
//   schemes which have parameters such as "pitch" which is the address
//   difference between two vertically adjacent pixels, and "offset[3]"
//   which has the offsets from the address of a pixel to the addresses
//   of the bytes containing red, green, and blue components.  I.e.
//   some formats can use either XY interleaving or Z stacking, just by
//   altering these parameters.
//
//   Only one "alloc" is done for all channels.  This is important to
//   know when freeing the memory.
//
//
// RETURN VALUES
//   Return -1 if failed, 0 otherwize.
//
//   If any of the image channels are non-NULL on input, a diagnostic
//   message is displayed.
//
// SEE ALSO
//   rgbaImageFree
*/
int rgbaImageAlloc(RgbaImageT *self, const int nx, const int ny)
{
    /* see whether there was un-freed memory here before */
    if(   (self->ri != NULL) || (self->gi != NULL)
            || (self->bi != NULL) || (self->ai != NULL) )
    {
        fprintf(stderr, "rgbaImageAlloc: warning: "
                "allocating over un-freed rgbaImage\n");
    }

    self->ncols = nx;
    self->nrows = ny;

    /* Make sure the image size is not zero */
    if((self->ncols * self->nrows) == 0)
    {
        fprintf(stderr, "rgbaImageAlloc: warning: zero size\n");
    }

    if((self->ri=MY_CALLOC(self->ncols * self->nrows * 4, unsigned char)) == NULL)
    {
        fprintf(stderr, "rgbaImageAlloc: Bad Alloc\n");
        return -1;
    }

    /* Find the beginning address for each of the image channels */
    self->gi = & ( self->ri[self->ncols * self->nrows * 1] );
    self->bi = & ( self->ri[self->ncols * self->nrows * 2] );
    self->ai = & ( self->ri[self->ncols * self->nrows * 3] );

    return 0 ;
}

/* NAME
//   rgbaImageDissolve: Dissolve two images
//
//
// ARGUMENTS
//   self (out): tween image.  Arrays allocated here.
//
//   siP (in): "source" image.
//
//   diP (in): "destination" image.
//     If diP is NULL then it is as if dest_image is black.
//
//   dissolve (in): dissolve parameter
//     where out = (1-dissolve) * source_image + dissolve * dest_image
//     e.g. if dissolve==0, out=source_image.  If dissolve==1, out=dest_image.
//
*/
int rgbaImageDissolve(RgbaImageT *self, const RgbaImageT *siP,
                  const RgbaImageT *diP, float dissolve)
{
    int nx;       /* image x-size */
    int xi;       /* loop image x-index */
    int yi;       /* loop image y-index */

    int rsi;      /* siP image red channel pixel value */
    int gsi;      /* siP image green channel pixel value */
    int bsi;      /* siP image blue channel pixel value */
    int asi;      /* siP image opacity channel pixel values */

    int rdi;      /* diP image red channel pixel value */
    int gdi;      /* diP image green channel pixel value */
    int bdi;      /* diP image blue channel pixel value */
    int adi;      /* diP image opacity channel pixel value */


    /* See whether diP image exists. */
    if(diP != NULL)
    {
        /* Make sure siP and diP images are the same size */
        if((siP->nrows != diP->nrows) || (siP->ncols != diP->ncols))
        {
            fprintf(stderr, "rgbaImageDissolve: input image size mismatch\n");
            return -1;
        }

        if(siP->compressed || diP->compressed)
            self->compressed = 1;

        self->pixel_size = MAX(siP->pixel_size, diP->pixel_size);

        if(siP->color_mapped && diP->color_mapped)
            self->color_mapped = 1;

    }
    else
    {
        if(siP->compressed)
            self->compressed = 1;

        self->pixel_size = siP->pixel_size;

        if(siP->color_mapped)
            self->color_mapped = 1;
    }

    /* Initialize the dissolved image */
    /* Note that this "undoes" the above code.
    // One wonders why I did it this way.
    */
    nx = siP->ncols;

    self->compressed = self->color_mapped = 0;
    self->pixel_size = 32;

    /* Allocate space for dissolved image data */

#if DEBUG >= 2
    printf("rgbaImageDissolve: allocating\n");
#endif

    if(rgbaImageAlloc(self, siP->ncols, siP->nrows))
        return -1;

    /* Dissolve the two images according to the dissolve parameter */
    for(yi=0; yi < self->nrows; yi++)
    {
        for(xi=0; xi < nx; xi++)
        {

            /* Compute contribution from siP image */
            rsi = (1.0-dissolve) * siP->ri[yi * nx + xi];
            gsi = (1.0-dissolve) * siP->gi[yi * nx + xi];
            bsi = (1.0-dissolve) * siP->bi[yi * nx + xi];
            asi = (1.0-dissolve) * siP->ai[yi * nx + xi];

            /* Compute contribution from diP image */
            if((diP!=NULL) && (xi<diP->ncols) && (yi < diP->nrows))
            {
                rdi = dissolve * diP->ri[yi * diP->ncols + xi];
                gdi = dissolve * diP->gi[yi * diP->ncols + xi];
                bdi = dissolve * diP->bi[yi * diP->ncols + xi];
                adi = dissolve * diP->ai[yi * diP->ncols + xi];
            }
            else
            {
                /* If there is no diP image, assume a black image instead */
                rdi = 0;
                gdi = 0;
                bdi = 0;
                adi = 0;
            }

            /* Compute the dissolved image pixel values */
            self->ri[yi*nx+xi] = (int)(rsi + rdi + 0.5);
            self->gi[yi*nx+xi] = (int)(gsi + gdi + 0.5);
            self->bi[yi*nx+xi] = (int)(bsi + bdi + 0.5);
            self->ai[yi*nx+xi] = (int)(asi + adi + 0.5);
        }
    }

    return 0;
}

/* NAME
//   rgbaImageRead: load image into memory.
//
//
// ARGUMENTS
//   self (in/out): pointer to RgbaImage
//
//   filename (in): filename
//
//
// DESCRIPTION
//   Frees old image channel space.
//   Allocates new image channel space.
*/
int rgbaImageRead(RgbaImageT *self, const char *filename)
{
    int           tga_return;
    tga_hdr_t     tga_hdr;
    FILE         *infP=NULL;

    /* Open the input file for binary reading */
    if(filename!=NULL && (infP=fopen(filename, "rb"))==NULL)
    {
        fprintf(stderr, "rgbaImageRead: could not open '%s' for input\n", filename);
        return -1;
    }

    /* Load the image header:
    // This will set 'self' members such as ncols, nrows, etc.
    */
    /* Targa */
    if( (tga_return = tgaHeaderRead(&tga_hdr, self, infP)) )
    {
        fprintf(stderr, "tgaHeaderRead returned %i\n", tga_return);
        return tga_return;
    }

    /* Free the memory for the previous image planes.
    // This must be done AFTER the load attempt, because if the load
    // fails, we want to keep the original image.
    */
    {
        int ncols = self->ncols; /* store geometry set by load_header */
        int nrows = self->nrows; /* store geometry set by load_header*/
        rgbaImageFree(self);     /* this sets ncols = nrows = 0 */
        self->ncols = ncols;     /* retrieve geometry */
        self->nrows = nrows;     /* retrieve geometry */
    }

    /* Allocate memory for the new image channels.
    // Note the unusual use of passing in self->ncols and self->nrows,
    // even though they are already set to the correct value.  This is
    // because tgaHeaderRead sets those values to the size of the image
    // about to be read in.
    */
    if(rgbaImageAlloc(self, self->ncols, self->nrows))
        return -1;

    /* Load the new image */
    /* Targa */
    tgaRead(&tga_hdr, self, infP);

    /* Close the input file */
    fclose(infP);

    return 0;
}

/* NAME
//   rgbaImageWrite: dissolve 2 images and save dissolved image to file
//
//
// ARGUMENTS
//   filename (in): file name to save image to
//
//   siP (in): "source" image pointer
//
//   diP (in): "destination" image pointer.
//     If diP is NULL then it is as if dest_image is black.
//
//   dissolve (in): dissolve parameter
//     where out = (1-dissolve) * source_image + dissolve * dest_image
//     e.g. if dissolve==0, out=source_image.  If dissolve==1, out=dest_image.
//
//
// DESCRIPTION
//   Dimensions of the output image are the same as the source_image.
//
//   "source" and "destination" do NOT refer to the disk space where the
//   image is being written.  They refer to the starting and finishing
//   images in the dissolve.
*/
int rgbaImageWrite(const char *filename, const RgbaImageT *siP,
               const RgbaImageT *diP, float dissolve)
{
    RgbaImageT  img;                /* temporary dissolved image */
    FILE        *outfP=NULL;        /* output file pointer */

    /* Dissolve the siP and diP images into img */
    rgbaImageInit(&img);
    if(rgbaImageDissolve(&img, siP, diP, dissolve))
    {
        return -1;
    }

    /* Open the output image file for binary writing */
    if(filename!=NULL && (outfP=fopen(filename, "wb"))==NULL)
    {
        fprintf(stderr, "rgbaImageWrite: could not open '%s' for output\n",
                filename);
        return -1;
    }

    {
        /* Set the image header */
        tga_hdr_t   tga_hdr;

        /* Targa */
        tga_hdr.id_len = 0;

        /* cmap_type depends on the img_type */
        tga_hdr.cmap_type = 0;

        /* img_type comes from the user */
        tga_hdr.img_type = TGA_RGB;

        if(img.compressed) tga_hdr.img_type += TGA_RLE;

        tga_hdr.cmap_index = 0;

        /* cmap_len depends on the img_type and pixel_size */
        tga_hdr.cmap_len = 0;

        /* cmap_size depends on the img_type and pixel_size */
        tga_hdr.cmap_size = 0;

        tga_hdr.x_off = 0;
        tga_hdr.y_off = 0;

        /* pixel_size depends on the img_type */
        tga_hdr.pixel_size = img.pixel_size;

        tga_hdr.att_bits = 0;
        tga_hdr.reserved = 0;
        tga_hdr.origin_bit = 0;
        tga_hdr.interleave = TGA_IL_None;

        /* Save the image header */
        {
            int         tga_return; /* return values from tgaHeaderWrite */
            /* Targa */
            if( (tga_return = tgaHeaderWrite(&tga_hdr, &img, outfP)) )
            {
                fprintf(stderr, "tgaHeaderWrite returned %i\n", tga_return);
                return tga_return;
            }
        }

        /* Save the dissolved image */
        /* Targa */
        tgaWrite(&tga_hdr, &img, outfP);
    }

    /* Free the dissolved image */
    rgbaImageFree(&img);

    /* Close the output image file */
    fclose(outfP);

    return 0;
}

/* rgbaImageTestCreate: generate a test image
//
//
// ARGUMENTS
//   self: RgbaImage instance
//   type: bitfield: which test image pattern to use.
//
//
// DESCRIPTION
//   Uses the incoming values of ncols and nrows to determine image size.
//   If ncols or nrows are zero, default values are used instead.
//
//   Memory for the images is allocated and 'self' is set.
*/
int rgbaImageTestCreate(RgbaImageT *self, int type)
{
    int           xi, yi;          /* pixel coordinate indices */
    unsigned char p;               /* pixel value */
    int           alloc_flag = 0;  /* whether to allocate an image */

    self->compressed = 1;
    self->color_mapped = 0;
    self->pixel_size = 24;
    self->type = TARGA_MAGIC;

    /* Test to see whether previous rgba image had any area.
    // If not, then create a default area.
    */
    if(self->ncols <= 0)
    {
        self->ncols = 300;
        alloc_flag = 1;
    }
    if(self->nrows <= 0)
    {
        self->nrows = 200;
        alloc_flag = 1;
    }

    /* Another possibility is that the size could have been set before
    // calling this routine, but no memory had yet been allocated.
    // In which case, memory ought to be allocated now.
    //
    // This might seem unusual-- allocating memory for the first time in
    // a routine which is not the object constructor.  But in a sense, this
    // is a RgbaImage constructor -- It generates an image, often for
    // the first time, simply to occupy screen space to indicate that the
    // image exists.  But sometimes, this routine is also used to simply
    // create a test image to erase a previous image, in which case this
    // routine does not act like a constructor.
    */
    if((self->ri == NULL) || (self->gi == NULL) || (self->bi == NULL))
    {
        alloc_flag = 1;
    }

    if(alloc_flag)
    {

#if DEBUG >= 2
        printf("rgbaImageTestCreate: Alloc %i %i\n", self->ncols, self->nrows);
#endif

        if(rgbaImageAlloc(self, self->ncols, self->nrows))
            return 1;
    }

    /* Create the test pattern */
    for(yi=0; yi < self->nrows; yi++)
    {
        for(xi=0; xi < self->ncols; xi++)
        {

            p = 15 + 240*((float)xi/self->ncols)*((float)yi/self->nrows);

            if((xi%40>20 && yi%40<20) || (xi%40<20 && yi%40>20))
                p=0;

            if(type & 1)
            {
                self->ri[yi*(self->ncols) + xi] = p;
            }
            else
            {
                self->ri[yi*(self->ncols) + xi] = RGBA_IMAGE_MAXVAL - p;
            }

            if(type & 2)
            {
                self->gi[yi*(self->ncols) + xi] = p;
            }
            else
            {
                self->gi[yi*(self->ncols) + xi] = RGBA_IMAGE_MAXVAL - p;
            }

            if(type & 4)
            {
                self->bi[yi*(self->ncols) + xi] = p;
            }
            else
            {
                self->bi[yi*(self->ncols) + xi] = RGBA_IMAGE_MAXVAL - p;
            }

            self->ai[yi*(self->ncols) + xi] = RGBA_IMAGE_OPAQUE;
        }
    }
    return 0;
}

/* RGBA_MESH_WARP:
// The following code is for warping RgbaImages.  The other RgbaImage
// code above does not require the use of the mesh code or the warp code,
// so the following code is enclosed in an ifdef block
*/
#ifdef RGBA_MESH_WARP

#include "mesh.h"
#include "warp.h"

int rgbaImageWarp(const RgbaImageT *img_orig, RgbaImageT *img_warp,
              const MeshT *mesh_src, const MeshT *mesh_dst, float tween_param)
{
    MeshT mesh_tween;

    meshInit(&mesh_tween);

    if(meshCompatibilityCheck(mesh_src, mesh_dst))
    {
        fprintf(stderr, "rgbaImageWarp: meshes are incompatible\n");
        return 1;
    }

    /* Set the tween mesh */
    meshAlloc(&mesh_tween, mesh_src->nx, mesh_src->ny);
    meshInterpolate(&mesh_tween, mesh_src, mesh_dst, tween_param);

    /* Allocate space for the warp image */
    rgbaImageFree(img_warp);
    if(rgbaImageAlloc(img_warp, img_orig->ncols, img_orig->nrows))
        return 1;

    /* Warp the image, one channel at a time */
    /* Warp forward from mesh_src to mesh_tween */

    warp_image(img_orig->ri, img_warp->ri, img_orig->ncols,
               img_orig->nrows, mesh_src->x, mesh_src->y, mesh_tween.x,
               mesh_tween.y, mesh_tween.nx, mesh_tween.ny);

    warp_image(img_orig->gi, img_warp->gi, img_orig->ncols,
               img_orig->nrows, mesh_src->x, mesh_src->y, mesh_tween.x,
               mesh_tween.y, mesh_tween.nx, mesh_tween.ny);

    warp_image(img_orig->bi, img_warp->bi, img_orig->ncols,
               img_orig->nrows, mesh_src->x, mesh_src->y, mesh_tween.x,
               mesh_tween.y, mesh_tween.nx, mesh_tween.ny);

    warp_image(img_orig->ai, img_warp->ai, img_orig->ncols,
               img_orig->nrows, mesh_src->x, mesh_src->y, mesh_tween.x,
               mesh_tween.y, mesh_tween.nx, mesh_tween.ny);

    meshUnref(&mesh_tween);

    return 0;
}

#endif /* RGBA_MESH_WARP */

/* RGBA_TK:
// The following code is for converting RgbaImages into Tk Photo images.
// The other RgbaImage code above does not require the use of Tk,
// so the following code is enclosed in an ifdef block.
*/
#ifdef RGBA_TK

#include <tcl.h>
#include <tk.h>

/* NAME
//   rgbaImageTkPhotoConvert: convert RgbaImageT to a TCL/Tk PhotoImage
*/
void rgbaImageTkPhotoConvert(RgbaImageT *self, Tk_PhotoHandle photoH)
{
    /* Give the image block to Tk */
    Tk_PhotoImageBlock block;

    block.pixelPtr  = self->ri;
    block.width     = self->ncols;
    block.height    = self->nrows;
    block.pitch     = self->ncols;
    block.pixelSize = sizeof(unsigned char);
    block.offset[0] = 0;
    block.offset[1] = self->ncols * self->nrows;
    block.offset[2] = 2 * block.offset[1];

    /* Set the photo image size */
    Tk_PhotoSetSize(photoH, block.width, block.height);

    Tk_PhotoPutBlock(photoH, &block, 0, 0, block.width, block.height);
}

#endif /* RGBA_TK */
