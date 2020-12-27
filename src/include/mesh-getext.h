#ifndef _MESH_GETEXT_H__INCLUDED_
#define _MESH_GETEXT_H__INCLUDED_

#include "mesh.h"

/************ EXTRA MESH GET FUNCTIONS  **************/



/*******************
 *  if you define   MESH_GET_EXT_NOT_SAFE
 *  then meshGetExt will not be safe for any choice of
 *  xi and yi: it will be safe only in a neighbourhood of the mesh
 *
 *  but it may be faster
 */


#ifdef MESHGETEXT_NOT_SAFE
#define MESHGETEXT__ONLY_Y__  __meshGetExt__only_y__
static inline double __meshGetExt__only_y__(MeshT *this, int xi, int yi,double *p);
#define MESHGETEXT__GETIT__  meshGet
#else
#define MESHGETEXT__ONLY_Y__  meshGetExt
#define MESHGETEXT__GETIT__  meshGetExt
#endif


static inline double meshGetExt(MeshT *this, int xi, int yi,double *p)
{
    if(xi<0 )
        return 2*MESHGETEXT__GETIT__(this,0,yi,p)
               -MESHGETEXT__ONLY_Y__(this, -xi,yi,p);
    else if (xi>=this->nx)
        return  2*MESHGETEXT__GETIT__(this,this->nx-1 ,yi,p)
                -MESHGETEXT__ONLY_Y__(this, 2*this->nx-xi-2,yi,p);
    if(yi<0 )
        return 2*MESHGETEXT__GETIT__(this,xi,0,p)
               -MESHGETEXT__GETIT__(this, xi,-yi,p);
    else if (yi>=this->ny)
        return  2*MESHGETEXT__GETIT__(this,xi,this->ny-1,p)
                -MESHGETEXT__GETIT__(this,xi, 2*this->ny-yi-2,p);
    return meshGet__no_check(this,xi,yi,p) ;
}

#ifdef MESHGETEXT_NOT_SAFE
static inline double __meshGetExt__only_y__(MeshT *this, int xi, int yi,double *p)
{
    if(yi<0 )
        return 2*MESHGETEXT__GETIT__(this,xi,0,p)
               -MESHGETEXT__GETIT__(this, xi,-yi,p);
    else if (yi>=this->ny)
        return  2*MESHGETEXT__GETIT__(this,xi,this->ny-1,p)
                -MESHGETEXT__GETIT__(this,xi, 2*this->ny-yi-2,p);
    return meshGet__no_check(this,xi,yi,p) ;
}
#endif

static inline double meshGetxExt2(MeshT *this, int xi, int yi)
{
    return meshGetExt(this,xi,yi, this->x);
}

static inline double meshGetyExt2(MeshT *this, int xi, int yi)
{
    return meshGetExt(this,xi,yi, this->y);
}

static inline double meshGetxExt(MeshT *this, int xi, int yi)
{
    double *p=this->x;
    if(xi<0 )
        return 2*MESHGETEXT__ONLY_Y__(this,0,yi,p)
               -MESHGETEXT__ONLY_Y__(this, -xi,yi,p);
    else if (xi>=this->nx)
        return  2*MESHGETEXT__ONLY_Y__(this,this->nx-1 ,yi,p)
                -MESHGETEXT__ONLY_Y__(this, 2*this->nx-xi-2,yi,p);
    if(yi<0 )
        return MESHGETEXT__GETIT__(this, xi,-yi,p);
    else if (yi>=this->ny)
        return MESHGETEXT__GETIT__(this,xi, 2*this->ny-yi-2,p);
    return meshGet__no_check(this,xi,yi,p) ;
}

static inline double meshGetyExt(MeshT *this, int xi, int yi)
{
    double *p=this->y;
    if(xi<0 )
        return MESHGETEXT__ONLY_Y__(this, -xi,yi,p);
    else if (xi>=this->nx)
        return MESHGETEXT__ONLY_Y__(this, 2*this->nx-xi-2,yi,p);
    if(yi<0 )
        return 2*MESHGETEXT__GETIT__(this,xi,0,p)
               -MESHGETEXT__GETIT__(this, xi,-yi,p);
    else if (yi>=this->ny)
        return  2*MESHGETEXT__GETIT__(this,xi,this->ny-1,p)
                -MESHGETEXT__GETIT__(this,xi, 2*this->ny-yi-2,p);
    return meshGet__no_check(this,xi,yi,p) ;
}

#endif // _MESH_GETEXT_H__INCLUDED_
