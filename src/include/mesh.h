/* mesh.h -- mesh handling routines header
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

#ifndef _MESH_H__INCLUDED_
#define _MESH_H__INCLUDED_

#include <stdio.h>

#include "braindead_msvc.h"

#include "mesh_t.h"

#define meshUnsaved(m) ((m)->changed >0)

#define meshAllocated(m) ((m) && (m)->x && (m)->y && (m)->label )

/* MP_PICK_DIST: farthest you may be from a Mesh point to pick it */
#define MP_PICK_DIST 8

/* NAME
//   meshInit: initialize MeshT members
//
// DESCRIPTION
//   meshInit should be called immediately after a MeshT is
//   instantiated or defined..
*/
inline static void meshInit(MeshT *this)
{
    this->nx = this->ny = 0;
    this->x  = this->y  = NULL;
    this->label = NULL;
    this->changed=0;
    this->reference_counting=0;
}

/* the situation was messy: sometimes the values for the point
 coordinates were float, but the values in the arrays are double
*/

int meshAlloc(MeshT *this, int nx, int ny);

MeshT * meshNew(const int nx, const int ny);

static inline void meshFree(MeshT *this);

void meshDelete(MeshT *this);

void meshPrint(const MeshT *this);

int meshCompatibilityCheck(const MeshT *this, const MeshT *other);

void meshInterpolate(MeshT *moP, const MeshT *m1P, const MeshT *m2P, double tween_param);

void meshReset(MeshT *this, const int img_width, const int img_height);

void meshScale(MeshT *this, const int img_width, const int img_height) ;
void meshScaleFreeformat(MeshT *this, const double scale_x , const double  scale_y);

int meshFunctionalize(MeshT *this, int img_width, int img_height);

long int meshPointNearest(const MeshT *this, int px, int py, int *mi, int *mj, int *dx, int *dy);

int meshPick(const MeshT *this, int mouse_x, int mouse_y, int component, double proximity);

void meshSet(MeshT *this, int xi, int yi, double new_x, double new_y);

static inline void meshSetNoundo(MeshT *this, int xi, int yi, double new_x, double new_y);

void meshSetLabel(MeshT *this, int xi, int yi, MESHLABEL_T new_label);

void meshCopy(MeshT *this, const MeshT *source) ;

void meshBackupIndexSet(int backup_index) ;

void meshBackupFree(void) ;

void meshStore(const MeshT *this) ;

void meshRetrieve(MeshT *this) ;

int meshLineAdd(MeshT *this, const int mi, const double mt, const int type);

int meshLineDelete(MeshT *this, int mi, int type);

int meshLineMouseModify(MeshT *this, MeshT *other, int mouse_x, int mouse_y, char line_type, char action);

int meshRead(MeshT *this, const char *filename);
int meshRead_stream(MeshT *this, FILE *fP);

int meshWrite(MeshT *this, char *filename);
int meshWrite_stream(MeshT *this, FILE *fP);

void meshMatch(MeshT *this, const MeshT *other);

double meshDistance(MeshT *this, const MeshT *other, int nolabel);

/*****************************************************************************
 * "get" functions for accessing points in meshes
 *****************************************************************************/

/* reflect the coordinate in the mesh to have it inside
 *(this must be a macro)
 */
#define  _MESH_H____reflect_coord_mesh(N,X) { if((X)<0) (X)=-(X);\
                                     (X) =(X) % (2*(N)-2);\
                                      if((X)>=N) (X)=2*(N)-(X)-2; }
/* controls if the coordinates in the mesh are inside */

#define  _MESH_H__check_coord_mesh_np(THIS,XI,YI) ( \
 ((XI) >= (THIS)->nx)  || ((XI) < 0) || ((YI) >= (THIS)->ny)  || ((YI) < 0))


#ifndef NDEBUG
#ifdef g_warning
#define  _MESH_H__check_coord_mesh_return0(THIS,XI,YI) {\
 if(_MESH_H__check_coord_mesh_np(THIS,XI,YI)) {\
    g_warning("coord out of mesh, in %s at line %d\n",__FILE__,__LINE__);\
 return 0;}}
#else  /*  g_warning */
#define  _MESH_H__check_coord_mesh_return0(THIS,XI,YI) {\
 if(_MESH_H__check_coord_mesh_np(THIS,XI,YI)) {\
 fprintf(stderr,"coords out of mesh, in %s at line %d\n",__FILE__,__LINE__);\
 return 0;}}
#endif /* g_warning */
#else /*  NDEBUG  */
#define  _MESH_H__check_coord_mesh_return0(THIS,XI,YI) {}
#endif /* NDEBUG */

#ifndef _MESH_H__CLAMP___
#define _MESH_H__CLAMP___(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

/********* generic "get" calls ************/

/* inline functions...
 *  when you would need a macro with local variables!
 */

#define meshGet__no_check(T,X,Y,P)   ((P)[(Y) * (T)->nx + (X)])

static inline double meshGet(MeshT *this, int xi, int yi,double *p)
{
    _MESH_H__check_coord_mesh_return0(this,xi,yi);
    return meshGet__no_check(this, xi,  yi,p) ;
}

static inline double meshGetClamp(MeshT *this, int xi, int yi,double *p)
{
    return meshGet__no_check(this,
                             _MESH_H__CLAMP___(yi,0,this->ny-1),
                             _MESH_H__CLAMP___(xi,0,this->nx-1),
                             p);
}

static inline double meshGetRefl(MeshT *this, int xi, int yi,double *p)
{
    _MESH_H____reflect_coord_mesh(this->nx,xi);
    _MESH_H____reflect_coord_mesh(this->ny,yi);
    return meshGet__no_check(this, xi,  yi,p);
}

/********* "get" x and y calls ************/
static inline double meshGetx(MeshT *this, int xi, int yi)
{
    return meshGet(this,xi,yi, this->x);
}

static inline double meshGety(MeshT *this, int xi, int yi)
{
    return meshGet(this,xi,yi, this->y);
}

static inline double meshGetxClamp(MeshT *this, int xi, int yi)
{
    return meshGetClamp(this,xi,yi, this->x);
}

static inline double meshGetyClamp(MeshT *this, int xi, int yi)
{
    return meshGetClamp(this,xi,yi, this->y);
}

static inline double meshGetxRefl(MeshT *this, int xi, int yi)
{
    return meshGetRefl(this,xi,yi, this->x);
}

static inline double meshGetyRefl(MeshT *this, int xi, int yi)
{
    return meshGetRefl(this,xi,yi, this->y);
}

/*******************************************************************/

/**
    minimum and maximum values for x,y in the mesh to be inside the mesh
**/
static inline double meshMinx(MeshT *mesh)
{
    return   meshGet__no_check(mesh,0,0,mesh->x);
}

static inline double meshMiny(MeshT *mesh)
{
    return   meshGet__no_check(mesh,0,0,mesh->y);
}

static inline double meshMaxx(MeshT *mesh)
{
    return   meshGet__no_check(mesh,mesh->nx-1,mesh->ny-1,mesh->x);
}

static inline double meshMaxy(MeshT *mesh)
{
    return   meshGet__no_check(mesh,mesh->nx-1,mesh->ny-1,mesh->y);
}

static inline MESHLABEL_T meshGetLabel(MeshT *this, int xi, int yi)
{
    _MESH_H__check_coord_mesh_return0(this,xi,yi);
    return meshGet__no_check(this, xi,yi,this->label);
}

static inline void meshSetNoundo(MeshT *this, int xi, int yi, double new_x, double new_y)
{
#ifndef NDEBUG
    if(_MESH_H__check_coord_mesh_np(this,xi,yi))
    {
#ifdef g_warning
        g_warning("set coord out of mesh, in %s at line %d\n",__FILE__,__LINE__);
#else
        fprintf(stderr,"\
set coord out of mesh, in %s at line %d\n",__FILE__,__LINE__);
#endif
    }
    else
#endif
    {
        this->x[yi * this->nx + xi] = new_x;
        this->y[yi * this->nx + xi] = new_y;
        this->changed ++;
    }
}

/* NAME
//   meshFree: free memory of internal arrays of a MeshT
//             THIS FUNCTION IS DEPRECATED: USE meshUnref
//
// ARGUMENTS
//   this: pointer to mesh
//
//
// NOTES
//   The memory of the MeshT instance is not freed here.
//
//   mennucci: since this is influenced by NDEBUG and the presence of glib,
//     I put it in mesh.h, so it is recompiled inside the applications using
//     libmorph
*/
void  meshFreeReally(MeshT *this);

static inline void meshFree(MeshT *this)
{

#ifndef NDEBUG_LIBMORPH_REF_COUNT
#ifndef NDEBUG
#ifdef g_warning
    g_warning("meshFree is not to be used in applications! use meshUnref\n");
#else /*g_warning*/
    fprintf(stderr,"meshFree is not to be used in applications! use meshUnref\n");
#endif /*g_warning*/
    if(this->reference_counting>0)
    {
#ifdef g_warning
        g_warning("meshFree: mesh has positive reference counting!\n");
#else /*g_warning*/
        fprintf(stderr,"meshFree: mesh has positive reference counting!\n");
#endif /*g_warning*/
#ifdef LIBMORPH_STRICTLY_CHECKS_REFERENCE_COUNTING
        abort();
#endif
    }
#endif /* NDEBUG*/
#endif /* NDEBUG_LIBMORPH_REF_COUNT*/
    meshFreeReally(this);
}


static inline void meshRef(MeshT *this)
{
    this->reference_counting++;
}

static inline void meshUnref(MeshT *this)
{
    this->reference_counting--;
    if(this->reference_counting==0)
        meshFreeReally(this);
}

struct mesh_variance_s
{
    double vx;
    double vy;
    double vxy;
    double mx;
    double my;
};

struct mesh_variance_s mesh_variance(MeshT *mesh, const int anylabel);
void mesh_normalize_variance(MeshT *mesh,const int anylabel,
                             const struct  mesh_variance_s bef,
                             const struct  mesh_variance_s aft );

#endif /* _MESH_H__INCLUDED_ */
