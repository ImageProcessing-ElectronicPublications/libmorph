/* mesh.c: mesh handling routines
//

   Written and Copyright (C) 1994-1999 by Michael J. Gourlay
   Modified  & Copyright (C) 1999-2002 by Andrea Mennucci

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

#include "my_malloc.h"
#include "mesh.h"

/* Mesh dimension limits
//
// The minimum has to do with the necessities of cubic spline fitting.
//
// The maximum is just a heuristic I figured was about right.
// The heuristic is that the number of mesh lines in either direction
// should not be more than half the number of pixels in that direction.
// More mesh lines than that would probably yield garbled images.
// Anyway, humans would not be able to deal with nearly that many
// mesh lines because they would be far too dense to keep organized.
*/
#define MESH_MIN_NX 4
#define MESH_MIN_NY 4
#define MESH_MAX_NX(this) (((this)->x[(this)->nx * (this)->ny - 1])/2)
#define MESH_MAX_NY(this) (((this)->y[(this)->nx * (this)->ny - 1])/2)

/* mesh_backup: backup copies of meshes for later "undo" operations
*/
#define NUM_MESH_BACKUPS 2
static MeshT mesh_backup[NUM_MESH_BACKUPS];
static int mesh_backup_index = 0;

#define SGN(x)   ( ((x)>0) ? (1) : ( ((x)<0) ? (-1) : (0)  ))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* NAME
//   meshAlloc: allocate memory for internal arrays of a MeshT
//
//
// ARGUMENTS
//   this (in/out): mesh.  this->nx and this->ny are used on input to
//     determine mesh size.  Memory for this->x and this->y are allocated.
//     this->nx and this->ny are set.
//
//   nx (in): number of mesh-points along x-direction
//
//   ny (in): number of mesh-points along y-direction
//
//
// DESCRIPTION
//   As each MeshT instance is created, it should be initialized
//   using this routine, even if the size is zero.
//
//   If the size is non-zero then memory for the mesh arrays is
//   allocated here.  If the size is zero then the mesh arrays are set
//   to NULL.
//
//
//   this routine automatically calls meshRef
// NOTES
//   The elements of the mesh arrays are double precision floating point
//   values.  The reason why double is used is that the image warping
//   routine, as it is currently implemented, requires double.
*/
int meshAlloc(MeshT *this, int nx, int ny)
{
    if(nx < 0 || ny < 0)
    {
        fprintf(stderr, "meshAlloc: ERROR: negative size: %i %i\n", nx, ny);
        return 1;
    }

    if(nx < MESH_MIN_NX)
    {
        fprintf(stderr,
                "meshAlloc: WARNING: nx=%i was too small.  Setting to %i\n",
                nx, MESH_MIN_NX);
        nx = MESH_MIN_NX;
    }

    if(ny < MESH_MIN_NY)
    {
        fprintf(stderr,
                "meshAlloc: WARNING: ny=%i was too small.  Setting to %i\n",
                ny, MESH_MIN_NY);
        ny = MESH_MIN_NY;
    }

    if((this->x != NULL) || (this->y != NULL) || (this->label != NULL))
    {
        fprintf(stderr,
                "meshAlloc: warning: allocating over un-freed mesh\n");

#if (DEBUG >= 1)
        abort();
#endif

    }

    /* Set the mesh size */
    this->nx = nx;
    this->ny = ny;

    if(nx * ny == 0)
    {
        /* Set arrays to NULL to indicate that they are not allocated */
        this->x = this->y = NULL;
        return 0;
    }

    /* Allocate mesh arrays */
    if((this->x=MY_CALLOC(nx * ny, double))==NULL)
    {
        fprintf(stderr, "meshAlloc: Bad Alloc\n");
        return 1;
    }

    this->x[0] = 0.0;  /* for alloc debugging */

    if((this->y=MY_CALLOC(this->nx * this->ny, double))==NULL)
    {
        FREE(this->x);
        fprintf(stderr, "meshAlloc: Bad Alloc\n");
        return 1;
    }

    if((this->label=MY_CALLOC(this->nx * this->ny, MESHLABEL_T))==NULL)
    {
        FREE(this->x);
        FREE(this->y);
        fprintf(stderr, "meshAlloc: Bad Alloc\n");
        return 1;
    }

    this->y[0] = 0.0;  /* for alloc debugging */

#if (DEBUG >= 2)
    printf("meshAlloc: %p %p\n", this->x, this->y);
#endif
    meshRef(this);
    return 0;
}

/* NAME
//   meshNew: allocate and initialize a MeshT instance and its arrays
//
//
// DESCRIPTION
//   A Mesh is a 2D array of coordinate values that are used to
//   indicate the locations of regions of an image.  When two meshes are
//   used together with an image warping algorithm, the meshes indicate
//   where regions of an image are to be warped.  Mesh-based image
//   warping is the foundation of one way of doing image morphing.
//
//
// SEE ALSO
//   meshAlloc
*/
MeshT * meshNew(const int nx, const int ny)
{
    MeshT *mesh = MY_CALLOC(1, MeshT);

    if(NULL == mesh)
    {
        return NULL;
    }

    if(nx * ny == 0)
    {
        meshInit(mesh);
    }
    else
    {
        meshAlloc(mesh, nx, ny);
    }

    return mesh;
}

/* NAME
//   meshFreeReally: free memory of internal arrays of a MeshT
//             THIS FUNCTION IS NOT TO BE USED IN APPLICATIONS: USE meshUnref
//             see the file README.libmorph
//
// ARGUMENTS
//   this: pointer to mesh
//
//
// NOTES
//   The memory of the MeshT instance is not freed here.
//
*/
void meshFreeReally(MeshT *this)
{
#if (DEBUG >= 2)
    printf("freeing mesh %p %p\n", this->x, this->y);
#endif

    if(this->x != NULL)
    {
        FREE(this->x);
        this->x = NULL;
    }

    if(this->y != NULL)
    {
        FREE(this->y);
        this->y = NULL;
    }

    if(this->label != NULL)
    {
        FREE(this->label);
        this->label = NULL;
    }
}

/* NAME
//   meshDelete: free MeshT instance
//
//
// ARGUMENTS
//   this (in/out): pointer to MeshT instance
//
//
// NOTES
//   Does NOT free internal mesh arrays
//
//FIXME mennucci: it is confusing, I would eliminate it
*/
void meshDelete(MeshT *this)
{
    FREE(this);
}

/* NAME
//   meshPrint: print some info about a mesh, for debugging
*/
void meshPrint(const MeshT *this)
{
    printf("size=%li,%li    max corner=%g,%g\n", this->nx, this->ny,
           this->x[this->nx * this->ny - 1],
           this->y[this->nx * this->ny - 1]);
}

/* NAME
//   meshCompatibilityCheck: make sure two meshes are compatible
//
//
// ARGUMENTS
//   this (in): pointer to this mesh
//
//   other (in): pointer to other mesh
//
//
// DESCRIPTION
//   Two meshes must be compatible in order to create a "tween" mesh.
//
//   In order for meshes to be compatible, they must have the same
//   number of points in each direction.
//
//   If both meshes are the "input" meshes for an interpolation, they
//   should also have the same maximum values, i.e., their corners
//   should be at the same places.  However, this routine will be used
//   to check for compatibility between input and output meshes, in
//   which case the output mesh will initially have no mesh point
//   values set.
//
//
// RETURN VALUES
//   Return nonzero if meshes do not match.
//   Return zero if they match.
*/
int meshCompatibilityCheck(const MeshT *this, const MeshT *other)
{
    if(this->nx != other->nx)
    {
        return 1;
    }
    else if(this->ny != other->ny)
    {
        return 2;
    }

#ifdef MESH_CHECK_CORNERS
    if(this->x[0] != other->x[0])
    {
        return 3;
    }
    else if(this->x[this->nx-1] != other->x[this->nx-1])
    {
        return 4;
    }
    else if(this->y[0] != other->y[0])
    {
        return 5;
    }
    else if(this->y[this->ny-1] != other->y[this->ny-1])
    {
        return 6;
    }
#endif /* MESH_CHECK_CORNERS */

    return 0;
}

/* NAME
//   meshChannelLinInterp: linear interpolation between two meshes, channel
//
//
// ARGUMENTS
//   mi1 (in): a channel of input mesh 1
//
//   mi2 (in): a channel of input mesh 2
//
//   nx (in): number of mesh points in x-direction
//
//   ny (in): number of mesh points in y-direction
//
//   mo (out): the respective channel of the output mesh
//
//   t (in): tween parameter:
//     when 0<t<1, mo = (1-t) * mi1 + t * mi2.
//     e.g. when t==0, mo = mi1.  when t==1, mo = mi2.
//
//
// DESCRIPTION
//   Note that this routine only operates on a single channel of the
//   meshes.  (A mesh has two channels: the list of x-values and the list
//   of y-values.)
//
//
// NOTES
//   MJG 18jul94:
//   The roundoff error here, although small, sometimes triggers the
//   bounds check in the spline evaluator.  The effect should be
//   harmless, though, if the spline evaluates slightly out of range.
*/
static void meshChannelLinInterp(const double *mi1, const double *mi2, int nx, int ny, double t, double *mo)
{
    int xi, yi;

    for(yi=0; yi < ny; yi++)
    {
        for(xi=0; xi < nx; xi++)
        {
            mo[yi * nx + xi] = (1.0-t) * mi1[yi * nx + xi] + t * mi2[yi * nx + xi];
        }
    }
}

/* NAME
//   meshInterpolate: interpolate meshes
//
//
// ARGUMENTS
//   m1p (in): "source" mesh pointer
//
//   m2p (in): "destination" mesh pointer
//
//   tween_param: (in) parameter indicating output mesh configuration.
//     0 < tween_param < 1
//     When tween_param == 0, moP is m1p.
//     When tween_param == 1, moP is m2p.
//
//   moP (out): "tween" mesh, somewhere between m1P and m2P.
//
//
// SEE ALSO
//   See meshChannelLinInterp for semantics of tween_param.
*/
void meshInterpolate(MeshT *moP, const MeshT *m1P, const MeshT *m2P, double tween_param)
{
    int m_c_c;

    if( (m_c_c = meshCompatibilityCheck(m1P, m2P)) )
    {
        fprintf(stderr, "meshInterpolate: input mesh sizes mismatch %i\n", m_c_c);
        return;
    }

    if( (m_c_c = meshCompatibilityCheck(m1P, moP)))
    {
        fprintf(stderr, "meshInterpolate: input mesh size mismatches output mesh %i\n", m_c_c);
        return;
    }

    meshChannelLinInterp(m1P->x, m2P->x, m1P->nx, m1P->ny, tween_param, moP->x);
    meshChannelLinInterp(m1P->y, m2P->y, m1P->nx, m1P->ny, tween_param, moP->y);
}

/* NAME
//   meshCopy: perform deep copy of MeshT members and array contents
//
//
// SEE ALSO
//   meshStore, meshRetrieve
*/
void meshCopy(MeshT *this, const MeshT *source)
{
    /* FIXME why? this only generates segfaults! remember to always init meshes!! */
    meshFreeReally(this);
    meshAlloc(this, source->nx, source->ny);
    memcpy(this->x, source->x, sizeof(double) * this->nx * this->ny);
    memcpy(this->y, source->y, sizeof(double) * this->nx * this->ny);
    memcpy(this->label, source->label, sizeof(MESHLABEL_T) * this->nx * this->ny);
}

/* NAME
//   meshBackupIndexSet: set index of mesh backup buffer for "undo" operations
//
//
// SEE ALSO
//   meshStore(), meshRetrieve(), meshBackupIndexGet()
*/
void meshBackupIndexSet(int backup_index)
{
    if((backup_index < 0) || (backup_index >= NUM_MESH_BACKUPS))
    {
        fprintf(stderr, "meshStore: backup_index=%i out of range\n",
                backup_index);
        return;
    }

    mesh_backup_index = backup_index;
}

/* NAME
//   meshBackupIndexGet: return appropriate value of mesh_backup_index
//
//
// ARGUMENTS
//   this_or_other (in): flag determing whether to return current
//     index, or index of "other" mesh.  Zero value means return index
//     for "this".  Non-zero value means return index for "other".
//
//
// DESCRIPTION
//   A problem would arise in situations such as when "meshLineDelete" or
//   "meshLineAdd" is called from meshLineMouseModify, where the caller will
//   modify 2 meshes at the same time.  Inside meshLineMouseModify, there is
//   an ambiguity about what convention the user might choose for which
//   mesh_backup_index corresponds to which mesh.  An explicit policy is
//   therefore established for which mesh_backup_index corresponds with
//   which mesh.
//
//   The mesh_backup_index policy is the following:  If a mesh modification
//   routine modifies only one mesh and a backup copy of the mesh is kept,
//   then the backup is stored of that mesh into the location at the current
//   mesh_backup_index.  If a mesh modification routine modifies 2 meshes
//   then the "this" mesh is stored at the current mesh_backup_index and the
//   other mesh is stored in an adjacent mesh_backup_index.  The value of
//   the adjacent index is chosen such that, if the current
//   mesh_backup_index value is even, the adjacent index is the next higher
//   value of the index, and if the current value of mesh_backup_index is
//   odd, then the adjacent index is the lower value.  For example, if
//   mesh_backup_index=1 then the adjacent index value is 0.  If
//   mesh_backup_index=0 then the adjacent index value is 1.
//
//   An alternative would be to associate a backup copy with the address of
//   the original mesh, but this would introduce garbage collection
//   nightmares.
//
//
// SEE ALSO
//   meshBackupIndexSet()
*/
int meshBackupIndexGet(const int this_or_other)
{
    if(this_or_other)
    {
        /* Return "this" index value */
        return mesh_backup_index;
    }
    else
    {
        /* Return the "other" index value */
        if((mesh_backup_index % 2) == 0)
        {
            /* The current mesh_backup_index is even so the other is the next value */
            return mesh_backup_index + 1;
        }
        else
        {
            /* The current mesh_backup_index is odd so the other is the prev value */
            return mesh_backup_index - 1;
        }
    }
}

void meshBackupFree(void)
{
    int im;

    for(im=0; im < NUM_MESH_BACKUPS; im++)
    {
        meshFreeReally(&mesh_backup[im]);
    }
}

/* NAME
//   meshStore: store a mesh in a holding buffer for later "undo"
//
//
// NOTES
//   Recognize that some meshes are temporary and internal anyway, so
//   that some mesh operations should not result in a backup copy.  It
//   should be the case that such internal temporary meshes make calls
//   to mesh modification routines which the user would never call, so
//   there should be no problem with conflicts in backup buffers.
//
//
// SEE ALSO
//   meshRetrieve(), meshCopy(), meshBackupIndexSet(),
//   meshBackupIndexGet()
*/
void meshStore(const MeshT *this)
{
#if VERBOSE >= 1
    printf("meshStore: %p into %i\n", this, mesh_backup_index);
#endif

    meshCopy(&mesh_backup[mesh_backup_index], this);
}

/* NAME
//   meshRetrieve: retrieve a mesh from a holding buffer
//
//
// SEE ALSO
//   meshStore(), meshCopy(), meshBackupIndexSet(),
//   meshBackupIndexGet()
*/
void meshRetrieve(MeshT *this)
{
    meshCopy(this, &mesh_backup[mesh_backup_index]);
}

/* NAME
//   meshEdgeAssert: make sure that Mesh edge values are on the image edge
*/
static void meshEdgeAssert(MeshT *this, const int img_width, const int img_height)
{
    int vi;

    /* Assert top and bottom edge */
    for(vi=0; vi < this->nx; vi++)
    {
        this->y[                            vi] = 0.0;
        this->y[(this->ny - 1) * this->nx + vi] = (img_height - 1);
    }

    /* Assert left and right edge */
    for(vi=0; vi < this->ny; vi++)
    {
        this->x[vi * this->nx                 ] = 0.0;
        this->x[vi * this->nx + (this->nx - 1)] = (img_width - 1);
    }
}

/* NAME
//   meshReset : set image warp mesh to be a regularly spaced mesh
//
//
// ARGUMENTS
//   this: (in/out) mesh pointer
//
//   img_width: width, in pixels, of the image that goes with this mesh
//
//   img_height: height, in pixels, of the image that goes with this mesh
//
//
// DESCTIPTION
//   Resets the mesh to a regular rectangular grid.
//
//   Stores a backup copy of the original mesh in the current mesh
//   backup buffer.
//
//
// SEE ALSO
//   meshScale, meshStore
*/
void meshReset(MeshT *this, const int img_width, const int img_height)
{
    int xi, yi;
    const double mp_dx = (double)(img_width - 1)  / (double)(this->nx - 1) ;
    const double mp_dy = (double)(img_height - 1) / (double)(this->ny - 1) ;

    if((NULL == this->x) || (NULL == this->y))
    {
        fprintf(stderr, "meshReset: ERR: no mesh arrays.  Allocate them.\n");
        return;
    }

    /* Save a backup of the original mesh for possible "undo" */
    meshStore(this);

    for(yi=0; yi < this->ny; yi++)
    {
        for(xi=0; xi < this->nx; xi++)
        {
            this->x[yi * this->nx + xi] = (double)((mp_dx * (double)xi ));
            this->y[yi * this->nx + xi] = (double)((mp_dy * (double)yi ));
            this->label[yi * this->nx + xi] = 0;
        }
    }

    meshEdgeAssert(this, img_width, img_height);
}

/* NAME
//   meshScale: rescale Mesh to fit new image size
//
//
// DESCRIPTION
//   meshScale rescales Mesh values to fit to the given image size.
//   This operation is useful only when than using an image which
//   exactly fits this mesh, except that the image was rescaled for
//   some reason.
//
//   meshScale will probably be called on a mesh each time a new image
//   is read which is associated with that mesh.  Since reading a new
//   image usually means that the mesh will also have to be re-read,
//   the meshScale is a fairly useless operation, except that it allows
//   the user to recover from accidentally changing the mesh because of
//   either accidentally changing the wrong image, or changing the
//   image before having saved the associated mesh.
//
//   Effectively, what meshScale does is to make the actual mesh
//   coordinate values irrelavent.  One approach I had considered
//   using was to always make the mesh coordinates range from 0.0 to
//   1.0 and then use the image width and height as scaling values.
//   This would make the meshes independent of image size and shape.
//   Either way requires about the same amount of code and this way
//   I do not have to rewrite the warp algorithm.
//
//   Calling this routine also stores a backup copy of the original
//   mesh in the current mesh backup buffer.
//
//
// SEE ALSO
//   meshStore, meshReset
*/
void meshScale(MeshT *this, const int img_width, const int img_height)
{
    int xi, yi;
    double scale_x = 1.0;  /* amount to scale x values */
    double scale_y = 1.0;  /* amount to scale y values */

    if((NULL == this->x) || (NULL == this->y))
    {
        fprintf(stderr, "meshReset: ERR: no mesh arrays.  Allocate them.\n");
        return;
    }

    scale_x = img_width  / this->x[this->ny * this-> nx - 1];
    scale_y = img_height / this->y[this->ny * this-> nx - 1];

    /* Save a backup copy of the orignal mesh for possible "undo" */
    meshStore(this);

    for(yi=0; yi < this->ny; yi++)
    {
        for(xi=0; xi < this->nx; xi++)
        {
            this->x[yi * this->nx + xi] *= scale_x;
            this->y[yi * this->nx + xi] *= scale_y;
        }
    }

    meshEdgeAssert(this, img_width, img_height);
}

/********
AM : in latest versions of libmorph, it is not necessay that the border
of the mesh be the border of the image. (gtkmorph  edits in this way)
So the scaling routine above is broken in two different ways :-)
 */
void meshScaleFreeformat(MeshT *this, const double scale_x , const double  scale_y)
{
    int xi, yi;

    if((NULL == this->x) || (NULL == this->y))
    {
        fprintf(stderr, "meshReset: ERR: no mesh arrays.  Allocate them.\n");
        return;
    }

    for(yi=0; yi < this->ny; yi++)
    {
        for(xi=0; xi < this->nx; xi++)
        {
            this->x[yi * this->nx + xi] *= scale_x;
            this->y[yi * this->nx + xi] *= scale_y;
        }
    }
}

/* NAME
//   meshFunctionalize : set image warp mesh to be functional and bounded
//
//
// ARGUMENTS
//   this (in/out): mesh pointer
//
//   img_width: width, in pixels, of the image associated with this mesh
//
//   img_height: height, in pixels, of the image associated with this mesh
//
//
// DESCRIPTION
// This routine only enforces vertical and horizontal functional lines.
// (I.e. lines can cross diagonally.)
//
// The problem with this routine is that if a point is out of its box,
// it is mathematically ambiguous whether that point should be moved,
// or whether the adjacent point should be moved.  Moving either fixes
// the "functionality", but usually there is an intuitive choice which
// this algorithm does not see.  This algorithm moves both points.
// To fix this problem, a heuristic could be employed to place a point
// within some weighted average of its neighbors.  Another posibility
// would be to generate the spline and use the values the spline is
// forced to use.  The problem with this is that the spline already
// expects functional data, so forcing a spline might break the
// spline.  Yet another possibility is to make a first-pass through
// the data to see which points need fixing, by the criteria used in
// this routine, along with an additional backwards criterion to ensure
// symmetry..  Then, the second pass would weight the changes according to
// which points require changes.  This would, at least, keep major
// crossovers effects localized (such as when a single point crosses
// over many points).
//
// This could be looked at as a bare-bones functionalizer-- one which
// simply guarentees that meshes are functional.  It is probably the
// job of another algorithm to make the mesh look more like what the
// user intended, but the user should have done what it intended in the
// first place...
//
//
// RETURN VALUE
//   Return number of changes.
//
//
// SEE ALSO
//   meshStore
*/
int meshFunctionalize(MeshT *this, int img_width, int img_height)
{
    register int xi, yi;
    double mxv, myv;
    int loop_change;
    int mesh_change=0;

    /* Save a backup copy of the orignal mesh for possible "undo" */
    meshStore(this);

    /* Repeat the mesh changes until the mesh stops changing */
    /* (but stop trying after a while to avoid long or infinite loops) */
    do
    {
        loop_change = 0;

        /* Force top and bottom edges to be at borders */
        for(xi=0; xi < this->nx; xi++)
        {
            if(this->y[xi] != 0)
            {
                this->y[xi] = 0;
                loop_change++;
            }
            if(this->y[(this->ny - 1) * this->nx + xi] != (img_height-1))
            {
                this->y[(this->ny - 1) * this->nx + xi] = img_height-1;
                loop_change++;
            }
        }

        this->y[0] = 0;
        for(yi=1; yi < this->ny; yi++)
        {
            /* Force left and right edges to be at borders */
            if(this->x[yi * this->nx + 0] != 0)
            {
                this->x[yi * this->nx + 0] = 0;
                loop_change++;
            }
            if(this->x[yi * this->nx + (this->nx-1)] != (img_width-1))
            {
                this->x[yi * this->nx + (this->nx-1)] = img_width-1;
                loop_change++;
            }

            /* Enforce functionality */
            for(xi=1; xi < this->nx; xi++)
            {
                /* make current point right of previous point */
                if(this->x[yi * this->nx + xi] <= this->x[yi * this->nx + (xi-1)])
                {
                    mxv = (this->x[yi* this->nx + xi] + this->x[yi* this->nx + (xi-1)])/2;
                    this->x[yi * this->nx + xi]     = mxv + 1;
                    this->x[yi * this->nx + (xi-1)] = mxv - 1;
                    loop_change++;
                }
                /* make current point below point in previous row */
                if(this->y[yi * this->nx + xi] <= this->y[(yi-1) * this->nx + xi])
                {
                    myv = (this->y[yi* this->nx + xi] + this->y[(yi-1)* this->nx + xi])/2;
                    this->y[yi * this->nx + xi]     = myv + 1;
                    this->y[(yi-1) * this->nx + xi] = myv - 1 ;
                    loop_change++;
                }

                /* make current point inside image boundary */
                if(this->x[yi * this->nx + xi] > (img_width - this->nx + xi))
                {
                    this->x[yi * this->nx + xi]  =  img_width - this->nx + xi;
                    loop_change ++;
                }
                /* make current point inside image boundary */
                if(this->y[yi * this->nx + xi] > (img_height - this->ny + yi))
                {
                    this->y[yi * this->nx + xi]  =  img_height - this->ny + yi;
                    loop_change ++;
                }
            }
        }
        if(loop_change) mesh_change++;
    }
    while ((mesh_change < (this->nx + this->ny)) && loop_change);

    return mesh_change;
}

/* NAME
//   meshPointNearest: find the nearest meshpoint and return square distance
//
//
// ARGUMENTS
//   this (in): mesh pointer
//   px: (in) mouse pointer x-coordinate (can be out of range)
//   py: (in) mouse pointer y-coordinate (can be out of range)
//   mi: (out) i-index of closest meshpoint
//   mj: (out) j-index of closest meshpoint
//   dx: (out) x distance of pointer from nearest meshpoint
//   dy: (out) y distance of pointer from nearest meshpoint
//
//
// DESCRIPTION
//   Set the indices of the meshpoint and the x,y distances.
//   Distances are dx=(px - this->x[]) , dy=(py - this->y[])
//
//
// RETURN VALUE
//   Returns square distance between pointer and meshpoint
*/
long int meshPointNearest(const MeshT *this, int px, int py, int *mi, int *mj, int *dx, int *dy)
{
    int      xi, yi;      /* loop indices of mesh array */
    int      m_dx;        /* x-distance from mouse to visited mesh point */
    int      m_dy;        /* y-distance from mouse to visited mesh point */
    long int m_d;         /* square distance from mouse to visited mesh point */
    long int m_d_min = 2000000; /* smallest square distance so far */

#ifdef DONT_MOVE_BORDER
    /* Guarentee p[xy] is in range */
    if(px < this->x[0])
    {
        px = this->x[0];
    }
    if(py < this->y[0])
    {
        py = this->y[0];
    }
    if(px > this->x[this->ny * this->nx-1])
    {
        px = this->x[this->nx * this->ny-1];
    }
    if(py > this->y[this->ny * this->nx-1])
    {
        py = this->y[this->nx * this->ny-1];
    }
#endif
    /* Scan all mesh points */
    for(yi=0; yi < this->ny; yi++)
    {
        for(xi=0; xi < this->nx; xi++)
        {
            /* Compute distance between current mesh point and mouse */
            m_dx = px - this->x[yi * this->nx + xi];
            m_dy = py - this->y[yi * this->nx + xi];
            m_d = m_dx * m_dx + m_dy * m_dy;

            /* See if this mesh point is the closest so far */
            if(m_d < m_d_min)
            {
                m_d_min = m_d;
                /* Remember the index of this mesh point */
                *mi = xi;
                *mj = yi;
                if(dx!=NULL) *dx = m_dx;
                if(dy!=NULL) *dy = m_dy;
            }
        }
    }

    return m_d_min ;
}

/* NAME
//   meshPick: find the nearest mesh point to the mouse and return index
//
//
// ARGUMENTS
//   this (in): msh pointer
//
//   mouse_x (in): mouse x location relative to the upper-left of the mesh
//
//   mouse_y (in): mouse y location relative to the upper-left of the mesh
//
//   component(in): which index component to return
//     0=>i, 1=>j,
//     2=>x distance between mouse and meshpoint, (obsolete)
//     3=>y distance between mouse and mesh point (obsolete)
//
//   proximity(in): distance mouse must be within to do a pick.
//     negative values indicate infinite distance.
//
//
// RETURN VALUES
//   Depends on the value of "component".  See the ARGUMENTS section for
//   details.
*/
int meshPick(const MeshT *this, int mouse_x, int mouse_y, int component, double proximity)
{
    int mesh_i_index;
    int mesh_j_index;
    int distance_x;
    int distance_y;
    int distance;

    meshPointNearest(this, mouse_x, mouse_y, &mesh_i_index, &mesh_j_index,
                     &distance_x, &distance_y);

    distance = sqrt(distance_x*distance_x + distance_y*distance_y);

    if((proximity < 0.0) || (distance < proximity))
    {
        if(0 == component)
        {
            return mesh_i_index;
        }
        else if(1 == component)
        {
            return mesh_j_index;

#if 0
        }
        else if(2 == component)
        {
            return distance_x;
        }
        else if(3 == component)
        {
            return distance_y;
#endif

        }
        else
        {
            /* Invalid component value */
            return -2;
        }
    }
    else
    {
        /* mouse is too far from a mesh point to matter */
        return -1;
    }
}

/* NAME
//   meshSet:  set a mesh point, given indices and values
//
//
// ARGUMENTS
//   this (in/out): pointer to MeshT
//
//   xi (in): x index of the mesh point to set
//
//   yi (in): y index of the mesh point to set
//
//   new_x (in): new x-coordinate value to set mesh point location
//
//   new_y (in): new y-coordinate value to set mesh point location
//
//
// DESCRIPTION
//   meshSet is a basic accessor function to set a mesh point location.
//   Using meshSet is preferable to simply setting the mesh coordinate
//   explicitly because meshSet is free to perform other operations,
//   such as saving the original mesh in a holding place in case the
//   user decides to "undo" the change later.
//
//   Stores a backup copy of the original mesh in the current mesh
//   backup buffer.
//
//
// SEE ALSO
//   meshStore
*/
void meshSet(MeshT *this, int xi, int yi, double new_x, double new_y)
{
    /* Save a backup copy of the orignal mesh for possible "undo" */
    meshStore(this);
    /*mark the fact that this mesh was changed*/
    this->changed ++;
    meshSetNoundo(this,xi,yi,new_x,new_y);
}

void meshSetLabel(MeshT *this, int xi, int yi,
             MESHLABEL_T new_label)
{
    this->label[yi * this->nx + xi] = new_label;
    /*mark the fact that this mesh was changed*/
    this->changed ++;
}

/* NAME
//   meshLineAdd: add a mesh line
//
//
// ARGUMENTS
//   this (in/out): mesh pointer
//
//   mi (in): upper left index of the quadrangle enclosing the new line
//     (i.e. mi is less than the index of the new line in the new mesh.)
//     For adding vertical lines, mi is the index of the nearby left column.
//     For adding horizontal lines, mi is the index of the nearby upper row.
//
//   mt (in): relative distance between the surrounding mesh lines
//
//   type (in): 1 for vertical lines or 2 for horizontal lines
//
//
// DESCRIPTION
//   Allocates memory for the new mesh arrays.
//   Sets the incoming mesh array pointer to the newly allocated array.
//   Frees the old mesh arrays.
//
//   Stores a backup copy of the original mesh in the current mesh
//   backup buffer.
//
//
// NOTES
//   Adding a mesh line has a subtlety concerning "location" that is
//   not an issue with deleting mesh lines or picking mesh points.
//   When adding a mesh line, it is not quite so simple to figure out
//   where the user is indicating to add the line because in general
//   the line could be quite curvy and twisted.  This algorithm tries
//   its best, but be careful when reading this routine and providing
//   its input arguments.
//
//
// RETURN VALUES
//   Return zero if okay.
//
//   Returns nonzero if fails.
//     Failure can happen if memory runs out or if a bad value for "type"
//     is provided, or if mi is out of bounds.
//
//
// SEE ALSO
//   meshStore, meshLineDelete, meshLineMouseModify
*/
int meshLineAdd(MeshT *this, const int mi, const double mt, const int type)
{
    int xi, yi;
    MeshT new;            /* place holder for new mesh info */

    meshInit(&new);

    /* Set up the new mesh size */
    switch(type)
    {
        /* Add vertical */
    case 1:
        /* Add a column */
        new.nx = this->nx + 1;
        new.ny = this->ny;
        if((mi<-1) || (mi > this->nx))
        {
            fprintf(stderr,"meshLineAdd: bad value: 0>mi=%i>nx=%li\n", mi,this->nx);
            return -2;
        }
        break;

        /* Add horizontal */
    case 2:
        /* Add a row */
        new.nx = this->nx;
        new.ny = this->ny + 1;
        if((mi<-1) || (mi > this->ny))
        {
            fprintf(stderr,"meshLineAdd: bad value: 0>mi=%i>ny=%li\n", mi,this->ny);
            return -3;
        }
        break;

        /* Invalid type */
    default:
        fprintf(stderr, "meshLineAdd: Bad Value: type: %i\n", type);
        return -1 ;
    }

    /* Allocate the new mesh */
    if(meshAlloc(&new, new.nx, new.ny))
        return 1 ;

    /* Save a backup copy of the orignal mesh for possible "undo" */
    meshStore(this);

    /* Make the change */
    switch(type)
    {
        /* --- Add vertical line --- */
    case 1:
        /* Copy the left columns from old into new */
        for(yi=0; yi < this->ny; yi++)
        {
            for(xi=0; xi <= mi; xi++)
            {
                new.x[yi * new.nx + xi] = this->x[yi * this->nx + xi];
                new.y[yi * new.nx + xi] = this->y[yi * this->nx + xi];
                new.label[yi * new.nx + xi] = this->label[yi * this->nx + xi];
            }
        }

        /* Copy the right columns from old into new */
        for(yi=0; yi < this->ny; yi++)
        {
            for(xi=mi+1; xi < this->nx; xi++)
            {
                new.x[yi * new.nx + (xi+1)] = this->x[yi * this->nx + xi];
                new.y[yi * new.nx + (xi+1)] = this->y[yi * this->nx + xi];
                new.label[yi * new.nx + (xi+1)] = this->label[yi * this->nx + xi];
            }
        }

        /* Add the new column */
        {
            double mx1, mx2, mxv;
            double my1, my2, myv;
            int mileft=CLAMP(mi,0,this->nx-1),
                miright=CLAMP(mi+1,0,this->nx-1);
            for(yi=0; yi < this->ny; yi++)
            {
                /* Place new line between two horizontally adjacent lines */
                mx1 = this->x[yi * this->nx + mileft];
                mx2 = this->x[yi * this->nx + miright];
                mxv = (1.0-mt) * mx1 + mt * mx2;
                new.x[yi * new.nx + (mi+1)] = mxv;

                my1 = this->y[yi * this->nx + mileft];
                my2 = this->y[yi * this->nx + miright];
                myv = (1.0-mt) * my1 + mt * my2;
                new.y[yi * new.nx + (mi+1)] = myv;
            }
        }
        break;

        /* --- Add horizontal line --- */
    case 2:
        /* Copy the top rows from old to new */
        for(yi=0; yi <= mi; yi++)
        {
            for(xi=0; xi< this->nx; xi++)
            {
                new.x[yi * new.nx + xi] = this->x[yi * this->nx + xi];
                new.y[yi * new.nx + xi] = this->y[yi * this->nx + xi];
                new.label[yi * new.nx + xi] = this->label[yi * this->nx + xi];
            }
        }

        /* Copy the bottom rows from old to new */
        for(yi=mi+1; yi < this->ny; yi++)
        {
            for(xi=0; xi < this->nx; xi++)
            {
                new.x[(yi+1) * new.nx + xi] = this->x[yi * this->nx + xi];
                new.y[(yi+1) * new.nx + xi] = this->y[yi * this->nx + xi];
                new.label[(yi+1) * new.nx + xi] = this->label[yi * this->nx + xi];
            }
        }

        /* Add the new row */
        {
            double mx1, mx2, mxv;
            double my1, my2, myv;
            int miup=CLAMP(mi,0,this->ny-1),
                midown=CLAMP(mi+1,0,this->ny-1);
            for(xi=0; xi < this->nx; xi++)
            {
                /* Place new line between two vertically adjacent lines */
                mx1 = this->x[(miup)   * this->nx + xi];
                mx2 = this->x[(midown) * this->nx + xi];
                mxv = (1.0-mt) * mx1 + mt * mx2;
                new.x[(mi+1) * new.nx + xi] = mxv;

                my1 = this->y[(miup)   * this->nx + xi];
                my2 = this->y[(midown) * this->nx + xi];
                myv = (1.0-mt) * my1 + mt * my2;
                new.y[(mi+1) * new.nx + xi] = myv;
            }
        }
        break;

        /* --- Invalid type --- */
    default:
        fprintf(stderr, "meshLineAdd: Bad Value: type: %i\n", type);
        return -1 ;
    }

    meshFreeReally(this);
    /* Free the old mesh arrays */

    /* Point to the new mesh arrays */
    this->x  = new.x;
    this->y  = new.y;
    this->nx = new.nx;
    this->ny = new.ny;
    this->label= new.label;
    this->changed++;
    return 0 ;
}

/* meshLineDelete: delete a mesh line
//
//
// ARGUMENTS
//   this: mesh pointer
//
//   mi: index the of a mesh point on the to-be-deleted line
//     for deleting vertical lines, mi is the index of the column
//     for deleting horizontal lines, mi is the index of the row
//
//   type: 1 for vertical lines or 2 for horizontal lines
//
//
// DESCRIPTION
//   Allocate memory for the mesh and set the incoming mesh pointers to
//   the newly allocated array
//
//   Caller must decrement the appropriate local analogy to nx or ny
//
//   Stores a backup copy of the original mesh in the current mesh
//   backup buffer.
//
//
// RETURN VALUES
//   Return nonzero if meshLineDelete fails
//
//
// SEE ALSO
//   meshStore, meshLineAdd, meshLineMouseModify
*/
int meshLineDelete(MeshT *this, int mi, int type)
{
    int xi, yi;
    MeshT new;

    meshInit(&new);

    switch(type)
    {
        /* Delete vertical */
    case 1:
        /* Delete a column */
        new.nx = this->nx - 1;
        new.ny = this->ny;
        break;

        /* Delete horizontal */
    case 2:
        /* Delete a row */
        new.nx = this->nx;
        new.ny = this->ny - 1;
        break;

        /* Invalid type */
    default:
        fprintf(stderr, "meshLineDelete: Bad Value: type: %i\n", type);
        return -1;
    }

    if(meshAlloc(&new, new.nx, new.ny))
        return 1;

    switch(type)
    {
        /* --- Delete vertical line --- */
    case 1:
        /* Copy the left columns */
        for(yi=0; yi < this->ny; yi++)
        {
            for(xi=0; xi<mi; xi++)
            {
                new.x[yi * new.nx + xi] = this->x[yi * this->nx + xi];
                new.y[yi * new.nx + xi] = this->y[yi * this->nx + xi];
                new.label[yi * new.nx + xi] = this->label[yi * this->nx + xi];
            }
        }

        /* Copy the right columns */
        for(yi=0; yi< this->ny; yi++)
        {
            for(xi=mi+1; xi< this->nx; xi++)
            {
                new.x[yi * new.nx + (xi-1)] = this->x[yi * this->nx + xi];
                new.y[yi * new.nx + (xi-1)] = this->y[yi * this->nx + xi];
                new.label[yi * new.nx + (xi-1)] = this->label[yi * this->nx + xi];
            }
        }

        break;

        /* --- Delete horizontal line --- */
    case 2:
        /* Copy the top rows */
        for(yi=0; yi<mi; yi++)
        {
            for(xi=0; xi< this->nx; xi++)
            {
                new.x[yi * new.nx + xi] = this->x[yi * this->nx + xi];
                new.y[yi * new.nx + xi] = this->y[yi * this->nx + xi];
                new.label[yi * new.nx + xi] = this->label[yi * this->nx + xi];
            }
        }

        /* Copy the bottom rows */
        for(yi=mi+1; yi< this->ny; yi++)
        {
            for(xi=0; xi< this->nx; xi++)
            {
                new.x[(yi-1) * new.nx + xi] = this->x[yi * this->nx + xi];
                new.y[(yi-1) * new.nx + xi] = this->y[yi * this->nx + xi];
                new.label[(yi-1) * new.nx + xi] = this->label[yi * this->nx + xi];
            }
        }

        break;

        /* --- --- --- Invalid type --- --- --- */
    default:
        fprintf(stderr, "meshLineDelete: Bad Value: type: %i\n", type);
        return -1;
    }

    /* Save a backup copy of the orignal mesh for possible "undo" */
    meshStore(this);

    /* Free the old mesh arrays */
    meshFreeReally(this);

    /* Point to the new mesh arrays */
    this->x  = new.x;
    this->y  = new.y;
    this->nx = new.nx;
    this->ny = new.ny;
    this->label = new.label;
    this->changed++;
    return 0;
}

/* NAME
//   meshLineMouseModify: Modify a mesh line using the mouse
//
//
// ARGUMENTS
//   this (in/out): pointer to mesh which is being pointed at.
//
//   other (in/out): pointer to other mesh which accompanies "this"
//     mesh.  Ignored if NULL.
//
//   mouse_x (in): x location of the mouse, relative to the mesh origin,
//     where the mesh origin refers to the upper-left corner of the
//     mesh, and has coordinate (0,0).
//
//   mouse_y (in): y location of the mouse, relative to the mesh origin,
//     where the mesh origin refers to the upper-left corner of the
//     mesh, and has coordinate (0,0).
//
//   line_type (in): 'h' => horizontal , 'v' => vertical
//
//   action (in): 'a' => add , 'd' => delete
//
//
// SEE ALSO
//   meshLineAdd, meshLineDelete, meshLineStore
*/
int meshLineMouseModify(MeshT *this, MeshT *other, int mouse_x,
                    int mouse_y, char line_type, char action)
{
    int mi, mj;   /* index of the mesh point nearest the mouse */
    int mdx, mdy; /* x and y distances between nearest mesh point and mouse */
    int md;       /* scalar distance between nearest mesh point and mouse */
    int mesh_backup_index_original = meshBackupIndexGet(0);

#if (DEBUG >= 1)
    fprintf(stderr, "meshLineMouseModify: %p %p %i %i %c %c\n",
            this, other, mouse_x, mouse_y, line_type, action);
#endif

    md = sqrt(meshPointNearest(this, mouse_x, mouse_y, &mi, &mj, &mdx, &mdy));

    if('a' == action)
    {
        /* Add mesh line */

        /* Make indices refer to upper left point in surrounding quadrangle */
        if( ( SGN(mdx) < 0 ) && ( mi > 0) )
        {
            mi --;
        }

        if( ( SGN(mdy) < 0 ) && ( mj > 0) )
        {
            mj --;
        }


        /* Test which line_type is being modified,
        // test whether the number of mesh lines is not too large,
        // test whether we are sitting away from an existing mesh line.
        // (We do not want to add another line on top of another one.)
        */
        if('v' == line_type)
        {
            if((this->nx < MESH_MAX_NX(this)) && (mdx != 0))
            {

                double mx1 = this->x[mj * this->nx + mi];
                double mx2 = this->x[mj * this->nx + (mi+1)];

                /* Find a place mid-way in between two surrounding mesh points.
                // mdx will be negative if the mouse is closer to the mi+1
                // mesh point, in which case mxt will also be negative, in which
                // case, we correct for this later.
                */
                double mxt = mdx / (mx2 - mx1);

                if(mxt < 0.0)
                {
                    mxt += 1.0;
                }

                /* Add a vertical mesh line at the mid-way place */
                meshLineAdd(this, mi, mxt, 1);

                /* Add vertical mesh line in corresponding place for other mesh */
                if(other != NULL)
                {
                    meshBackupIndexSet(meshBackupIndexGet(1));
                    meshLineAdd(other, mi, mxt, 1);
                    meshBackupIndexSet(mesh_backup_index_original);
                }

            }

        }
        else if('h' == line_type)
        {
            if((this->ny < MESH_MAX_NY(this)) && (mdy != 0))
            {

                double my1 = this->y[mj     * this->nx + mi];
                double my2 = this->y[(mj+1) * this->nx + mi];
                double myt = mdy / (my2 - my1);

                if(myt < 0.0)
                {
                    myt += 1.0;
                }

                meshLineAdd(this, mj, myt, 2);

                if(other != NULL)
                {
                    meshBackupIndexSet(meshBackupIndexGet(1));
                    meshLineAdd(other, mj, myt, 2);
                    meshBackupIndexSet(mesh_backup_index_original);
                }

            }
        }
        else
        {
            fprintf(stderr, "meshLineMouseModify: ERROR: invalid line_type '%c'\n",
                    line_type);
        }

    }
    else if ('d' == action)
    {
        /* Delete mesh line */

        if(md >= MP_PICK_DIST)
        {
            /* Mouse is too far from any mesh point to be meaningful */

#if (DEBUG >= 1)
            fprintf(stderr,
                    "meshLineMouseModify: mouse distance = %i > %i too far for delete\n",
                    md, MP_PICK_DIST);
#endif

            return -1;
        }

        /* Test line type for vertical or horizontal,
        // make sure that we are not trying to delete the left-most mesh line,
        // make sure that we are not trying to delete the right-most mesh line,
        // make sure that we are not going to end up with too few mesh lines.
        */
        if('v' == line_type)
        {
            if((mi>0) && (mi<(this->nx - 1)) && (this->nx > MESH_MIN_NX))
            {
                meshLineDelete(this, mi, 1);
                if(other != NULL)
                {
                    meshLineDelete(other, mi, 1);
                }
            }
        }
        else if('h' == line_type)
        {
            if((mj>0) && (mj<(this->ny - 1)) && (this->ny > MESH_MIN_NY))
            {
                meshLineDelete(this, mj, 2);
                if(other != NULL)
                {
                    meshLineDelete(other, mj, 2);
                }
            }
        }
        else
        {
            fprintf(stderr, "meshLineMouseModify: ERROR: invalid line_type '%c'\n",
                    line_type);
        }

    }
    else
    {
        fprintf(stderr, "meshLineMouseModify: ERROR: invalid action, '%c'\n",
                action);
        return 1;
    }

    return 0;
}

/* NAME
//   meshRead: Read a mesh from a file
//
//
// ARGUMENTS
//   this: (in/out) mesh pointer
//
//   filename: (in) mesh file name
//
//
// DESCRIPTION
//   Frees memory of previous mesh.
//   Allocates memory for the meshes and sets .nx and .ny members.
//
//
// RETURN VALUES
//   Returns zero if load succeeds.
//   Returns nonzero if load fails.
// NOTES
//   If file is too short, it destroyes the old mesh!
//
//   Now it saves integers, otherwise it is affected by the locale
//    and this is very bad. This format is though backward compatible.
//    To keep some subpixel precision,the points are multiplied by 10 !
*/
int meshRead(MeshT *this, const char *filename)
{
    int b;
    FILE  *fP;            /* mesh file pointer */
    /* Open mesh file for reading */
    if((fP=fopen(filename, "r"))==NULL)
    {
        fprintf(stderr, "meshRead: could not read file '%s'\n", filename);
        return 1;
    }
    b=meshRead_stream(this,fP);
    fclose(fP);
    return b;
}

int meshRead_stream(MeshT *this, FILE  *fP)
{
    int    xi, yi;        /* loop mesh indices */
    int    nx = -1;       /* number of mesh points along x, read from file */
    int    ny = -1;       /* number of mesh points along y, read from file */
    /*double mesh_point;     input buffer for mesh point value */
    char   magic[2];      /* magic number, for file identification */

    char s[250]; /* string for parsing */


    /* Read first two characters of mesh file */
    if(fread(magic, 1, 2, fP) < 2)
    {
        fprintf(stderr, "meshRead: premature EOF in file\n");
        return EOF;
    }

    /* "M2" as the first two characters indicates an ASCII mesh file */
    if(magic[0]=='M' && magic[1]=='2')
    {

        /* Read the mesh geometry */
        if(fscanf(fP, "%i", &nx)!=1 || (nx < 0))
        {
            fprintf(stderr, "meshRead: missing or bad nx: %i\n", nx);
            return 2;
        }

        if(fscanf(fP, "%i", &ny)!=1 || (ny < 0))
        {
            fprintf(stderr, "meshRead: missing or bad ny: %i\n", ny);
            return 3;
        }

        /* Free the old mesh and allocate memory for the new mesh */
        meshFreeReally(this);
        meshInit(this);
        if(meshAlloc(this, nx, ny))
        {
            return 6;
        }
        /*this reads the newline */
        fgets(s, 249, fP);

        /* Read the mesh point values */
#ifdef TRANSPOSE_MESH
        for(xi=0; xi < this->nx; xi++)
            for(yi=0; yi < this->ny; yi++)
#else
        for(yi=0; yi < this->ny; yi++)
            for(xi=0; xi < this->nx; xi++)
#endif
            {
                /* this was modified to read files with or without labels */
                if(fgets(s, 249, fP) == NULL)
                {
                    fprintf(stderr, "meshRead: missing line at %i %i\n", xi, yi);
                    /* no: meshFreeReally(this); rather, returns what is */
                    return 4;
                }
                {
                    int a=
                        sscanf(s,"%lf %lf %d",
                               &(this->x[yi * this->nx + xi] ),
                               &(this->y[yi * this->nx + xi] ),
                               &(this->label[yi * this->nx + xi]));
                    this->x[yi * this->nx + xi]/=10.0;
                    this->y[yi * this->nx + xi]/=10.0;
                    if( a<2)
                    {
                        fprintf(stderr, "\
meshRead: only %d args in line at %i %i\n\
line is: '%s'.\n", a,xi, yi,s);
                        /* no: meshFreeReally(this); rather, tries on
                        return 4;*/
                    }
                }
            }
    }
    else
    {
        fprintf(stderr, "meshRead: file was not a valid mesh file\n");
        return 5;
    }

    return 0;
}

/* NAME
//   meshWrite: save a mesh to file named filename
//
//
// ARGUMENTS
//   this: (in) mesh pointer
//   filename: (in) mesh file name
//
//
// RETURN VALUES
//   Returns zero if load succeeds.
//   Returns nonzero if load fails.
*/

int meshWrite(MeshT *this, char *filename)
{
    int b;
    FILE *fP;

    if((fP=fopen(filename, "w"))==NULL)
    {
        fprintf(stderr, "meshWrite: could not write file '%s'\n", filename);
        return 1;
    }
    b=meshWrite_stream(this,fP);
    fclose(fP);
    return b;
}

int meshWrite_stream(MeshT *this, FILE *fP)
{
    int xi, yi;
    /* M2 indicates an ASCII mesh file */
    fprintf(fP, "M2\n");

    /* Write the mesh geometry */
    fprintf(fP, "%li %li\n", this->nx, this->ny);

    /* Write the mesh point values .
       Now it saves integers, otherwise it is affected by the locale
       and this is very bad. This format is though backward compatible.
       NOTE The points are multiplied by 10 !!
    */
    for(yi=0; yi < this->ny; yi++)
    {
        for(xi=0; xi < this->nx; xi++)
            fprintf(fP, "%d %d %d\n", (int) (10.0*this->x[yi * this->nx + xi]),
                    (int)(10.0*this->y[yi * this->nx + xi]),
                    this->label[yi * this->nx + xi]);
    }

    /*mark the fact that this mesh was saved*/
    this->changed=0;
    return 0;
}

/* NAME
//   meshMatch: Make mesh dimensions match another mesh
//
//
// ARGUMENTS
//   this: (in/out) mesh to be made to match the other mesh
//   other: (in) mesh to be matched to
//
//
// DESCRIPTION
//   If this mesh needs to be resized, then it is also reset.
//   This should probably be rewritten to simply rescale the other mesh,
//   instead of destroying it.
//
//   Stores a backup copy of the original mesh in the current mesh
//   backup buffer.
*/
void meshMatch(MeshT *this, const MeshT *other)
{

    if((this->nx != other->nx) || (this->ny != other->ny))
    {

#ifdef DEBUG
        printf("meshMatch: about to wreck the other mesh.  This needs fixing.\n");
#endif

        /* Save a backup copy of the orignal mesh for possible "undo" */
        meshStore(this);

        meshFreeReally(this);
        meshAlloc(this, other->nx, other->ny);

        /* The +1 is because meshReset wants width and height , but the
        // mesh stores pixel coordinates, which range from 0 to size-1 in
        // each direction.  The +0.5 is to avoid roundoff truncation.
        */
        meshReset(this,
                  other->x[other->nx * other->ny - 1] + 1.5,
                  other->y[other->nx * other->ny - 1] + 1.5);
    }
}

/* NAME
//   meshDistance :
//
//
// DESCTIPTION
//  computes distance of points with label != nolabel
*/

static inline double SQR(double A)
{
    return (A)*(A);
}

double meshDistance(MeshT *this, const MeshT *other, int nolabel)
{
    int xi, yi;
    double dist=0;

    if (!this || !other || meshCompatibilityCheck(this, other))
    {
        fprintf(stderr,"Incompatible meshes!! 982749812\n");
        return 0;
    }


    for(yi=0; yi < this->ny; yi++)
    {
        for(xi=0; xi < this->nx; xi++)
        {
            if (     this->label[yi * this->nx + xi] != nolabel
                     && other->label[yi * this->nx + xi] != nolabel )
                dist+=
                    SQR(this->x[yi * this->nx + xi]  - other->x[yi * other->nx + xi] )+
                    SQR(this->y[yi * this->nx + xi]  - other->y[yi * other->nx + xi] );
        }
    }

    return sqrt(dist);
}

/*****************************************************************************/

struct mesh_variance_s mesh_variance(MeshT *mesh, const int anylabel)
{
    struct mesh_variance_s  v;
    double x,y;
    int c=0,xi,yi;
    v.vx=0;
    v.vy=0;
    v.vxy=0;
    v.mx=0;
    v.my=0;
    for(xi=0; xi < mesh->nx ; xi++)
    {
        for(yi=0; yi<mesh->ny  ; yi++)
        {
            if( anylabel || 0 ==  meshGetLabel(mesh,xi,yi))
            {
                v.mx  += (x=meshGetx(mesh, xi,yi));
                v.my  += (y=meshGety(mesh, xi,yi));
                v.vx  += x*x;
                v.vy  += y*y;
                v.vxy += x*y;
                c++;
            }
        }
    }
    v.mx /= c;
    v.my /= c;
    v.vx /= c;
    v.vy /= c;
    v.vxy /= c;
    v.vx -= v.mx*v.mx;
    v.vy -= v.my*v.my;
    v.vxy -= v.mx*v.my;

    return v;
}

/* FIXME this is far from perfectly doing what advertised */
void mesh_normalize_variance___(MeshT *mesh,const int anykind,
                                const struct  mesh_variance_s bef,
                                const struct  mesh_variance_s aft )
{
    int xi,yi;
    double x,y,
           lx= sqrt(bef.vx/aft.vx),  ly=sqrt(bef.vy/aft.vy);
    for(xi=0; xi < mesh->nx ; xi++)
    {
        for(yi=0; yi<mesh->ny  ; yi++)
        {
            if( anykind || 0 ==  meshGetLabel(mesh,xi,yi))
            {
                x=(meshGetx(mesh, xi,yi)-aft.mx) *lx + bef.mx ;
                y=(meshGety(mesh, xi,yi)-aft.my) *ly + bef.my;
                meshSetNoundo(mesh,xi,yi,x,y);
            }
        }
    }
}

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef  ABS
#define ABS(a)     (((a) < 0) ? -(a) : (a))

#include "assert.h"
/* maximum eigenvalue */
static double __maxl__(const struct  mesh_variance_s v)
{
    /*
      (t-vx) (t-vy) -vxy vxy = tt - t(vx +vy)+ vx vy - vxy vxy
    */
    double b= -v.vx-v.vy,  c=v.vx*v.vy - v.vxy * v.vxy ,
           d= b*b -4 *c, l0 = 0.5 * ( -b + sqrt(d)),
           l1=  0.5 * ( -b - sqrt(d));
    assert(d>=0);
    return MIN(ABS(l1),ABS(l0));
}

static inline double __deter__(const struct  mesh_variance_s v)
{
    return v.vx*v.vy - v.vxy * v.vxy;
}

void mesh_normalize_variance(MeshT *mesh,const int anykind,
                             const struct  mesh_variance_s bef,
                             const struct  mesh_variance_s aft )
{
    int xi,yi;
    double x,y, bef_l=__deter__(bef), aft_l= __deter__(aft),
                lx= sqrt(bef_l/aft_l), ly=lx;

    for(xi=0; xi < mesh->nx ; xi++)
    {
        for(yi=0; yi<mesh->ny  ; yi++)
        {
            if( anykind || 0 ==  meshGetLabel(mesh,xi,yi))
            {
                x=meshGetx(mesh, xi,yi) *lx -aft.mx *lx + aft.mx ;
                y=meshGety(mesh, xi,yi) *ly -aft.my *ly + aft.my;
                meshSetNoundo(mesh,xi,yi,x,y);
            }
        }
    }
}
