
#ifndef _MESH_T_H__INCLUDED_
#define _MESH_T_H__INCLUDED_

#define MESHLABEL_T int
#define MESHPOINTSELECTED (-1)

typedef struct
{
    long    nx;   /* number of mesh points in the x-direction */
    long    ny;   /* number of mesh points in the y-direction */
    double *x;    /* 2D array of mesh point x-values */
    double *y;    /* 2D array of mesh point y-values */
    MESHLABEL_T *label; /* any point may be labelled , for grouping points  etc
	      the most important labels are
	     0 == normal point
	     -1 == selected point */
    unsigned int changed; /*if it has been changed since the last saving */
    /* the reference count: good for porting libmorph in python (or
       any language that needs garbage collecting)
       for easier use in GTK . read README.libmorph */
    unsigned int reference_counting;
} MeshT;

#endif /* _MESH_T_H__INCLUDED_ */
