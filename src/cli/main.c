/* main.c : morph main program
//
// A GUI or command line user interface to a mesh warping algorithm
//


   Written and Copyright (C) 1994-2000 by Michael J. Gourlay

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
#include <string.h>
#include <math.h>

#include "mesh.h"
#include "rgbaimage.h"

#define NUM_ORIG_IMAGES 2

#define cl_match_arg(cmd) (!strcmp(argv[apc], cmd) && ((apc+1)<argc))

#define XT_OPTIONS "[Xt options]"

#define OPTIONS "\n" \
"     options:                          (default value is in parentheses)\n" \
"       -start  starting_image_file     (test image)\n" \
"       -finish finishing_image_file    (test image)\n" \
"       -out    output_image_file       (warp0000.tga)\n" \
"       -src    source_mesh_file        (uniform grid)\n" \
"       -dst    destination_mesh_file   (uniform grid)\n" \
"       -mt     morph_tween             (0)             [between 0 and 1]\n" \
"       -dt     dissolve_tween          (0)             [between 0 and 1]\n"

#define NON_GUI_OPTIONS \
"       -skip_warp                      (false)\n"

#define USAGE "usage:  %s [options] " OPTIONS NON_GUI_OPTIONS

#ifndef FALSE
#define FALSE 0
#endif

int verbose = FALSE;

/* Global "original" images */
RgbaImageT orig_image[NUM_ORIG_IMAGES];

static float
sigmoid(float x)
{
    static const float SHARPNESS  =  2.0;
    float as         = atan(SHARPNESS);
    return (atan((x - 0.5) * SHARPNESS * 2.0) + as) / (2.0 * as);
}

int main(int argc, char **argv)
{
    char          *src_img_fn     = NULL;
    char          *dst_img_fn     = NULL;
    char          *src_mesh_fn    = NULL;
    char          *dst_mesh_fn    = NULL;
    char          *out_img_fn     = "warp0000.tga";
    RgbaImageT    *src_imgP       = &orig_image[0];
    RgbaImageT    *dst_imgP       = &orig_image[1];
    float          morph_tween    = 0.0;
    float          dissolve_tween = 0.0;
    int            skip_warp      = 0;
    int            apc;

#ifdef SUNOS
    malloc_debug(2);
#endif

    rgbaImageInit(src_imgP);
    rgbaImageInit(dst_imgP);

    for(apc=1; apc < argc; apc++)
    {
        if(argv[apc][0] != '-')
        {
        }
        else
        {
            if(cl_match_arg("-start"))
            {
                src_img_fn = argv[++apc];
            }
            else if(cl_match_arg("-finish"))
            {
                dst_img_fn = argv[++apc];
            }
            else if(cl_match_arg("-out"))
            {
                out_img_fn = argv[++apc];
            }
            else if (cl_match_arg("-src"))
            {
                src_mesh_fn = argv[++apc];
            }
            else if (cl_match_arg("-dst"))
            {
                dst_mesh_fn = argv[++apc];
            }
            else if (cl_match_arg("-mt"))
            {
                morph_tween = atof(argv[++apc]);
            }
            else if (cl_match_arg("-dt"))
            {
                dissolve_tween = atof(argv[++apc]);
                if(dissolve_tween < 0.0)
                {
                    dissolve_tween = sigmoid(-dissolve_tween);
                }
            }
            else if(!strcmp(argv[apc], "-verbose"))
            {
                verbose ++;
                fprintf(stderr, "%s: verbose reporting\n", argv[0]);
            }
            else if(!strcmp(argv[apc], "-skip_warp"))
            {
                skip_warp = 1;
            }
            else
            {
                fprintf(stderr, USAGE, argv[0]);
                return 1;
            }
        }
    }

    if(src_img_fn==NULL && dst_img_fn!=NULL)
    {
        fprintf(stderr,
                "%s: must have start image if finish image is given.\n", argv[0]);
        fprintf(stderr, USAGE, argv[0]);
        return 1;
    }

    /* Load the source image or create a test pattern image */

    if(src_img_fn != NULL)
    {
        if(rgbaImageRead(src_imgP, src_img_fn))
        {
            fprintf(stderr, "%s: could not open src image '%s'\n",
                    argv[0], src_img_fn);
            return 1;
        }
    }
    else
    {
        /* Create test pattern image.
        //
        // The unusual size of 319x239 is chosen specifically because it
        // will force xmorph to perform some sort of resizing as soon as the
        // user loads in a mesh or an image.  The size 320x240 is very common
        // and if no resizing was done by xmorph then I would not have a
        // thorough test of its resizing routines.
        */
        rgbaImageAlloc(src_imgP, 319, 239);
        rgbaImageTestCreate(src_imgP, 2);
    }

    /* Load the destination image */

    if(dst_img_fn != NULL)
    {
        if(rgbaImageRead(dst_imgP, dst_img_fn))
        {
            fprintf(stderr, "%s: could not open dst image '%s'\n",
                    argv[0], src_img_fn);
            return 1;
        }

        /* Make sure images are the same shape */
        if((dst_imgP->ncols != src_imgP->ncols)
                || (dst_imgP->nrows != src_imgP->nrows))
        {
            fprintf(stderr, "%s: images are not the same size\n", argv[0]);
            return 1;
        }
    }
    else
    {
        rgbaImageAlloc(dst_imgP, src_imgP->ncols, src_imgP->nrows);
        rgbaImageTestCreate(dst_imgP, 1);
    }

    /* Command line interface */

    if(verbose) fprintf(stderr, "morph_tween   : %g\n", morph_tween);
    if(verbose) fprintf(stderr, "dissolve_tween: %g\n", dissolve_tween);

    {
        MeshT *src_mesh = meshNew(4,4);
        MeshT *dst_mesh = meshNew(4,4);

        if (NULL != src_mesh_fn)
        {
            if(meshRead(src_mesh, src_mesh_fn))
            {
                fprintf(stderr, "%s: ERROR: failed to read src mesh '%s'\n",
                        argv[0], src_mesh_fn);
                rgbaImageFree(src_imgP);
                rgbaImageFree(dst_imgP);
                meshDelete(src_mesh);
                meshDelete(dst_mesh);
                return 1;
            }
        }
        else
        {
            meshReset(src_mesh, src_imgP->ncols, src_imgP->nrows);
        }

        meshScale(src_mesh, src_imgP->ncols, src_imgP->nrows);

        if (NULL != dst_mesh_fn)
        {
            if(meshRead(dst_mesh, dst_mesh_fn))
            {
                fprintf(stderr, "%s: ERROR: failed to read dst mesh '%s'\n",
                        argv[0], dst_mesh_fn);
                rgbaImageFree(dst_imgP);
                rgbaImageFree(dst_imgP);
                meshFree(src_mesh);
                meshDelete(src_mesh);
                meshDelete(dst_mesh);
                return 1;
            }
        }
        else
        {
            meshReset(dst_mesh, dst_imgP->ncols, dst_imgP->nrows);
        }

        meshScale(dst_mesh, dst_imgP->ncols, dst_imgP->nrows);

        {
            RgbaImageT *src_warped    = rgbaImageNew();
            RgbaImageT *dst_warped    = rgbaImageNew();


            if(!skip_warp)
            {
                if(verbose) printf(".");
                rgbaImageWarp(src_imgP, src_warped, src_mesh, dst_mesh, morph_tween);
                if(verbose) printf(".");
                rgbaImageWarp(dst_imgP, dst_warped, dst_mesh, src_mesh, morph_tween);
                if(verbose) printf(".");
                rgbaImageWrite(out_img_fn, src_warped, dst_warped, dissolve_tween);
                if(verbose) printf(".\n");
            }
            else
            {
                rgbaImageWrite(out_img_fn, src_imgP, dst_imgP, dissolve_tween);
            }

            rgbaImageFree(dst_warped);
            rgbaImageDelete(dst_warped);
            rgbaImageFree(src_warped);
            rgbaImageDelete(src_warped);
        }

        meshBackupFree();
        meshFree(src_mesh);
        meshDelete(src_mesh);
        meshFree(dst_mesh);
        meshDelete(dst_mesh);

    }

    rgbaImageFree(src_imgP);
    rgbaImageFree(dst_imgP);

    return 0;
}
