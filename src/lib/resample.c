#include "math.h"
#include "resample.h"
#include "strings.h"
/* periodic symmetric extension of src */
#define REFOLD(J,L)   { if(J<0) J =-J; J = J % (2*L-1); if(J>=L) J=(2*L-1)-J;}
/* border extension of src */
#define BORD(J,L)   { if((J)<0) (J)=0; else if ((J)>(L)) (J)=(L); }
/* zero extension of src */
#define ZERO(J,L)   { if(J<0 || J>L) continue; }

/* enforce boundary condition <-> array bounds */
/* #define BC(J,L) REFOLD(J,L) */

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef  ABS
#define ABS(a)     (((a) < 0) ? -(a) : (a))

#undef  CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#include "stdio.h"
#include "math.h"

/* void (*resample_array_inv_choices[10])(const double *F,  */
/*    const PIXEL_TYPE *src, int s_len, int s_stride,  */
/*    PIXEL_TYPE *dst, int d_len, int d_stride); */

typedef  void (* resample_t)(const double *F,
                             const PIXEL_TYPE *src, int s_len, int s_stride,
                             PIXEL_TYPE *dst, int d_len, int d_stride);

/*****************************************************************/

/*  WARNING this is not ANTIALIASED in any way but is very simple and fast */

static inline void resample_array_inv_near_neighbor
(const double *F,
 const PIXEL_TYPE *src, int s_len, int s_stride,
 PIXEL_TYPE *dst, int d_len, int d_stride)
{
    int i,p=0,j;
    for(i=0; i<d_len; i++)
    {
        j=F[i];
        BORD(j,s_len-1)
        dst[p]=src[j* s_stride];
        p+= d_stride;
    }
}

/*********** antialiased using bilinear interpixel**************
 * works well when the scale is not reduced and/or when there are not
 * very fine grains; it is much faster; it is approximate in many points
 * For animations it is better to use antialiased
 */

static void resample_array_inv_bilinear
(const double *F,
 const PIXEL_TYPE *src, int s_len, int s_stride,
 PIXEL_TYPE *dst, int d_len, int d_stride)
{
    int i,p=0,j,nj;
    double v,x,dj;
    for(i=0; i<d_len; i++)
    {
        x=F[i];
        BORD(x,s_len-1);
        j=floor(x);
        dj=x-j;
        nj=j+1;
        if(nj>=s_len)
            v=src[j* s_stride];
        else
            v=src[j* s_stride]*(1-dj) + src[nj* s_stride]*dj;
        dst[p]=v;
        p+= d_stride;
    }
}

#ifdef SEEMS_UNNEEDED
int c;
/* I have tested this improvement to
the above, and this seems to introduce ripples;
 I don't want to improve it, use AA if you really want AA*/
if( oj>=j+2)
{
    c=oj-j+1;
    if(oj>=s_len) oj=s_len-1;
    while (oj>=j)
    {
        v+=src[oj* s_stride];
        oj--;
    }
    v/=c;
}
else if ( oj+2<=j)
{
    c=j-oj+1;
    if(oj<0) oj=0;
    while (oj<=j)
    {
        v+=src[oj* s_stride];
        oj++;
    }
    v/=c;
}
else
#endif

    /*********** antialiased using convolutional kernels***************/

#if 256 >= (PIXEL_MAX-PIXEL_MIN)
#include "sinc_256.h"
#else
#include "sinc_1024.h"
#endif

#ifndef M_PI
#define M_PI 3.141
#endif

static double sinc(const double x)
{
    if (x<0.0001 && x > -0.0001)
        return 1;
    else
    {
        double tmp = M_PI * x  ;
        return sin(tmp) / tmp;
    }
}

static double sinc_by_table(const double x)
{
    if (x < -4 || x > 4)
    {
        double tmp = M_PI * x  ;
        return sin(tmp) / tmp;
    }
    else
        return sinc_table [(int)((ABS(x))*SINC_TABLE_UNIT )];
}

static double lanczos(const double x)
{
    if (x < -2 || x > 2)
        return 0;
    else
        return sinc(x);
}

static double lanczos4(const double x)
{
    if (x < -4 || x > 4)
        return 0;
    else
        return sinc(x);
}

static double triangle(const double x)
{
    if (x<-1) return 0;
    else if(x<0) return x+1;
    else if(x<1) return 1-x;
    else return 0;
}

/* this generates the function name */
#define FUN(A) XFUN(A)
#define XFUN(A) resample_array_inv_ ## A

/*************  choice of antialiasing kernel **************/
/* optimizations  for the case of any kernel  based on sinc
   suggested by lvalero, oberger 05/05/2004 */
#define KERNEL_sinc_fast

/* if the above is undefined, then this will be used instead
 this also is used in creating the function name*/
#define KERNEL lanczos

/* this is the half of the width of the kernel
 for lanczos4, it must be 4, otherwise 2*/
#define KERNEL_WIDTH 2

/* this creates the function */
#include "resample_snippet.h"

/* then another function */
#undef KERNEL
#define KERNEL lanczos4
#undef KERNEL_WIDTH
#define KERNEL_WIDTH 4
#include "resample_snippet.h"
/***********end  choice of antialiasing kernel **************/

resample_t resample_choices[10] =
{
    FUN(near_neighbor) ,     FUN(bilinear),
    FUN(lanczos),FUN(lanczos4),
    NULL
};

char * resample_array_inv_names[10] =
{
    "near_neighbor" , /* choose nearest pixel: fastest, looks bad */
    "bilinear", /* bilinear: same as with the old libmorph warping code */
    "lanczos", /* Lanczos:  much better quality, a must for animations
	       //and/or fine grained images; it is though  slower,*/
    "lanczos4",/* even better than before, but no noticeable difference on
		   most images */
    NULL
};

void (*resample_array_inv)(const double *F,
                           const PIXEL_TYPE *src, int s_len, int s_stride,
                           PIXEL_TYPE *dst, int d_len, int d_stride)= FUN(lanczos);

void mesh_resample_choose_aa(int f)
{

    resample_array_inv=resample_choices[f];
}

#include <string.h> /* for strcmp */

void mesh_resample_choose_aa_by_name(char * s)
{
    int f=0;
    while(resample_choices[f])
    {
        if (0==strcmp (s,resample_array_inv_names[f]))
        {
            resample_array_inv=resample_choices[f];
            return;
        }
        f++;
    }

    fprintf(stderr,"\n%s:%d: no choice '%s' for kernel!\n",__FILE__,__LINE__,s);
}
