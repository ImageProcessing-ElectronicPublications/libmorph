
#include "stdio.h"

#include "mesh.h"

/* safe get functions; these also define the border condition */
#define MESHGETXSAFE meshGetxExt
#define MESHGETYSAFE meshGetyExt

#define MESHGETEXT_NOT_SAFE
#include "mesh-getext.h"

#include "math.h"

#include "assert.h"

#undef	MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#undef	MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#undef	ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#undef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define SMOOTH_ITERATIONS 10


/*********
	   this routine will smooth out the mesh

	   it will only move points whose label is 0
*************/

/* relaxing step */
/*e#define STEP(CEN,WEST,EST,SOUTH,NORTH) (((NORTH)+(SOUTH)+(EST)+(WEST)) / 4.0)*/
#define STEPY(CEN,WEST,EST,SOUTH,NORTH) (((NORTH)*O+(SOUTH)*O+(EST)+(WEST)) / (2*O+2.0))
#define STEPX(CEN,WEST,EST,SOUTH,NORTH) (((NORTH)+(SOUTH)+(EST)*O+(WEST)*O) / (2*O+2.0))

/* this is now unused */
static double smooth_elastic_mesh_once
(MeshT *mesh,
 int dontoverlap , /* if true, a point cannot enter a neighbouring cell
		      but the algo is far from perfect in this respect*/
 int keepborder,  /* if true, dont' move border points */
 double O,/* orthogonal versus parallel smoothing;
	     1 is anisotropic, ie laplacian*/
 double maxx, double maxy
)
{
    int xi, yi;
    double x,y,ox,oy;
    double change=0;


#ifdef PRESERVE_VARIANCE
    struct mesh_variance_s bef, aft;
    const int anykind=0;
    bef=mesh_variance(mesh, anykind);
#endif

    /* bail out if mesh was externally deleted (thread safeness) */
    if(mesh->nx<=2 || mesh->ny<=2 )
        return change;

    for(xi=0; xi < mesh->nx ; xi++)
    {
        for(yi=0; yi<mesh->ny  ; yi++)
        {
            if( 0 ==  meshGetLabel(mesh,xi,yi))
            {
                if ( keepborder && xi == 0 )
                    ox=x=0;
                else if ( keepborder &&  xi == mesh->nx -1)
                    ox=x=maxx;
                else
                {
                    ox=meshGetx(mesh, xi,yi);
                    x=STEPX(ox,
                            MESHGETXSAFE(mesh, xi+1,yi) ,
                            MESHGETXSAFE(mesh, xi-1,yi) ,
                            MESHGETXSAFE(mesh, xi,yi+1) ,
                            MESHGETXSAFE(mesh, xi,yi-1)) ;
                    if(dontoverlap)
                    {
                        if ( x< MESHGETXSAFE(mesh, xi-1,yi))
                            x= MESHGETXSAFE(mesh, xi-1,yi);
                        else if(x> MESHGETXSAFE(mesh, xi+1,yi))
                            x= MESHGETXSAFE(mesh, xi+1,yi);
                        else if ( x< MESHGETXSAFE(mesh, xi-1,yi+1))
                            x= MESHGETXSAFE(mesh, xi-1,yi+1);
                        else if(x> MESHGETXSAFE(mesh, xi+1,yi-1))
                            x= MESHGETXSAFE(mesh, xi+1,yi-1);
                    }
                }

                if ( keepborder &&   yi == 0 )
                    oy=y=0;/* meshGety(mesh, xi,yi); */
                if ( keepborder &&  yi == mesh->ny -1)
                    oy=y=maxy; /* meshGety(mesh, xi,yi); */
                else
                {
                    oy=meshGety(mesh, xi,yi);
                    y=STEPY(oy,
                            MESHGETYSAFE(mesh, xi+1,yi),
                            MESHGETYSAFE(mesh, xi-1,yi),
                            MESHGETYSAFE(mesh, xi,yi+1),
                            MESHGETYSAFE(mesh, xi,yi-1)) ;
                    if(dontoverlap)
                    {
                        if ( y< MESHGETYSAFE(mesh, xi,yi-1))
                            y= MESHGETYSAFE(mesh, xi,yi-1);
                        else if(y> MESHGETYSAFE(mesh, xi,yi+1))
                            y= MESHGETYSAFE(mesh, xi,yi+1);
                        else if ( y< MESHGETYSAFE(mesh, xi-1,yi-1))
                            y= MESHGETYSAFE(mesh, xi-1,yi-1);
                        else if(y> MESHGETYSAFE(mesh, xi+1,yi+1))
                            y= MESHGETYSAFE(mesh, xi+1,yi+1);
                    }
                }
                meshSetNoundo(mesh,xi,yi,x,y);
                change+=sqrt((x-ox)*(x-ox) + (y-oy)*(y-oy));
            }
        }
    }
#ifdef PRESERVE_VARIANCE
    aft=mesh_variance(mesh, anykind);
    mesh_normalize_variance(mesh,anykind,bef,aft);
#endif

    return change;
}

double smooth_elastic_mesh(MeshT *mesh,
                           /* if true, a point cannot enter a neighbouring cell
                              but the algo is far from perfect in this respect*/
                           int dontoverlap,
                           int keepborder,/* if true, dont' move border points */
                           double O,
                           double maxx,double maxy
                          )
{
    int lp;
    double change=0;

    for(lp =SMOOTH_ITERATIONS ; lp ; lp--)
        change+=smooth_elastic_mesh_once(mesh,dontoverlap,keepborder,O,maxx,maxy);
    return change;
}

/******************************
relaxing step with a rubber wrt other mesh labels
***/

/* relaxing step */
#define STEP3DX(CEN,WEST,EST,SOUTH,NORTH,UP) ((2*(UP)+(NORTH)+(SOUTH)+(EST)*O+(WEST)*O) / (2*O+4.0))
#define STEP3DY(CEN,WEST,EST,SOUTH,NORTH,UP) ((2*(UP)+(NORTH)*O+(SOUTH)*O+(EST)+(WEST)) / (2*O+4.0))

static double smooth_mesh_rubber_once(MeshT *mesh, MeshT *rubber, double rubberish,
                               int i,int j,int label, int dontoverlap,int keepborder, double O)
{
    int xi, yi;
    double x,y,ox,oy;
    double  r=1.0-rubberish;
    double dx=meshGetx(mesh, i,j)-meshGetx(rubber, i,j),
           dy=meshGety(mesh, i,j)-meshGety(rubber, i,j);
    double change=0;

#ifdef PRESERVE_VARIANCE
    struct mesh_variance_s bef, aft;
    const int anykind=0;
    bef=mesh_variance(mesh, anykind);
#endif

    for(xi=0; xi < mesh->nx ; xi++)
    {
        for(yi=0; yi<mesh->ny  ; yi++)
        {
            if( label ==  meshGetLabel(mesh,xi,yi) &&
                    (xi != i || yi != j))
            {

                if ( keepborder && (  xi == 0 || xi == mesh->nx -1))
                    ox=x=meshGetx(mesh, xi,yi);
                else
                {
                    ox=meshGetx(rubber, xi,yi);
                    x=rubberish*
                      STEP3DX(ox,
                              MESHGETXSAFE(mesh, xi+1,yi) ,
                              MESHGETXSAFE(mesh, xi-1,yi) ,
                              MESHGETXSAFE(mesh, xi,yi+1) ,
                              MESHGETXSAFE(mesh, xi,yi-1) ,
                              meshGetx(rubber, xi,yi))
                      +r * (dx+meshGetx(rubber, xi,yi));
                    if(dontoverlap)
                    {
                        if ( x< MESHGETXSAFE(mesh, xi-1,yi))
                            x= MESHGETXSAFE(mesh, xi-1,yi);
                        else if(x> MESHGETXSAFE(mesh, xi+1,yi))
                            x= MESHGETXSAFE(mesh, xi+1,yi);
                        else if ( x< MESHGETXSAFE(mesh, xi-1,yi+1))
                            x= MESHGETXSAFE(mesh, xi-1,yi+1);
                        else if(x> MESHGETXSAFE(mesh, xi+1,yi-1))
                            x= MESHGETXSAFE(mesh, xi+1,yi-1);
                    }
                }

                if ( keepborder && ( yi == 0 || yi == mesh->ny -1))
                    oy=y=meshGety(mesh, xi,yi);
                else
                {
                    oy=meshGety(rubber, xi,yi);
                    y=rubberish*
                      STEP3DY(oy,
                              MESHGETYSAFE(mesh, xi+1,yi),
                              MESHGETYSAFE(mesh, xi-1,yi),
                              MESHGETYSAFE(mesh, xi,yi+1),
                              MESHGETYSAFE(mesh, xi,yi-1),
                              meshGety(rubber, xi,yi))
                      +r * (dy+meshGety(rubber, xi,yi));
                    if(dontoverlap)
                    {
                        if ( y< MESHGETYSAFE(mesh, xi,yi-1))
                            y= MESHGETYSAFE(mesh, xi,yi-1);
                        else if(y> MESHGETYSAFE(mesh, xi,yi+1))
                            y= MESHGETYSAFE(mesh, xi,yi+1);
                        else if ( y< MESHGETYSAFE(mesh, xi-1,yi-1))
                            y= MESHGETYSAFE(mesh, xi-1,yi-1);
                        else if(y> MESHGETYSAFE(mesh, xi+1,yi+1))
                            y= MESHGETYSAFE(mesh, xi+1,yi+1);
                    }
                }
                meshSetNoundo(mesh,xi,yi,x,y);
                change+=sqrt((x-ox)*(x-ox) + (y-oy)*(y-oy));
            }
        }
    }

#ifdef PRESERVE_VARIANCE
    aft=mesh_variance(mesh, anykind);
    mesh_normalize_variance(mesh,anykind,bef,aft);
#endif

    return change;
}

double smooth_mesh_rubber(MeshT *mesh, MeshT *rubber, double rubberish,
                          int i,int j,int label, int dontoverlap,int keepborder, double O)
{
    int lp;
    double change=0;
    assert(mesh->nx>=2 && mesh->ny>=2);
    for(lp =SMOOTH_ITERATIONS ; lp ; lp--)
    {
        change+=
            smooth_mesh_rubber_once(mesh,rubber,rubberish,i,j,label,dontoverlap,keepborder,O);
    }
    return change;
}

/***************************************************/

const static double D2[3][3][3] =
{
    {{ 0, 0, 0},{-1,2,-1},{0, 0, 0}},
    {{ 0,-1, 0},{ 0,2,0 },{0,-1, 0}},
    {{-1, 0, 1},{ 0,0,0 },{1, 0,-1}}
};

/* note that , if mesh is edited while it is smoothed, this will
  print useless warnings
  #define SELFTEST
*/

static double smooth_thin_plate(MeshT *mesh, int keepborder ,
                                double maxx, double maxy)
{
    int x1,y1,x2,y2, lx,ly, i,j,  k ,  lp;
    const int nx=mesh->nx, ny=mesh->ny;
    double x,y;
    double ox,oy;
    double meshx_D2[3][nx+2][ny+2];
    double meshy_D2[3][nx+2][ny+2];

#ifdef SELFTEST
    double ene=0, oldene=0;
#endif
    double change=0, partial_change=0;

#ifdef PRESERVE_VARIANCE
    const int anykind=0;
    struct mesh_variance_s   bef=mesh_variance(mesh, anykind) ,  aft;
#endif
#ifdef RECOMPUTE_CHANGE
    MeshT copymesh;
    meshInit(&copymesh);
    meshAlloc(&copymesh,nx,ny);
    meshCopy(&copymesh,mesh);
#endif

    const double derder_E=16;
#if 0
    /* the above is the result of this computation */
    for(i=0; i <= 2 ; i++)
        for(j=0; j <= 2  ; j++)
            for (k=0; k<2; k++)
                derder_E += D2[k][i][j] * D2[k][i][j];
#endif

#define COMPUTE_D2(X1,Y1) \
  {for (k=0;k<2;k++) { \
	meshx_D2[k][X1][Y1]=0;\
	meshy_D2[k][X1][Y1]=0; }\
   for (k=0;k<2;k++) { \
	for(i=0; i <= 2 ; i++)\
	  for(j=0; j <= 2  ; j++) {\
	    double t=D2[k][i][j];if(t) {\
	  int xxx=X1-i,yyy=Y1-j;\
    meshx_D2[k][X1][Y1]+=MESHGETXSAFE(mesh, xxx,yyy)* t;\
    meshy_D2[k][X1][Y1]+=MESHGETYSAFE(mesh, xxx,yyy)* t;\
	    }}}}

    /* compute D2 */
    for(x1=0; x1 < nx+2 ; x1++)
        for(y1=0; y1 < ny+2  ; y1++)
        {
            COMPUTE_D2(x1,y1);
#ifdef SELFTEST
            for (k=0; k<2; k++)
                ene+=(meshx_D2[k][x1][y1]*meshx_D2[k][x1][y1]+
                      meshy_D2[k][x1][y1]*meshy_D2[k][x1][y1]);
#endif
        }
#ifdef SELFTEST
    oldene=ene;
#endif

    /* repeat */
    for(lp = SMOOTH_ITERATIONS ; lp ; lp--)
    {
        partial_change=0;
        /*  bail out if mesh was externally deleted (thread safeness) */
        if(mesh->nx<=2 || mesh->ny<=2 )
            return change;
        for(lx=0; lx < nx ; lx++)
            for(ly=0; ly < ny  ; ly++)
            {
                if( 0  ==  meshGetLabel(mesh,lx,ly))
                {
                    double derEx=0,derEy=0;
                    x=meshGetx(mesh, lx,ly);
                    y=meshGety(mesh, lx,ly);
                    ox=x;
                    oy=y;

                    for (k=0; k<2; k++)
                        for(x1=0; x1 <= 2 ; x1++)
                            for(y1=0; y1 <= 2  ; y1++)
                            {
                                double t=D2[k][x1][y1];
                                if(t)
                                {
                                    derEx+= meshx_D2[k][lx+x1][ly+y1] * t;
                                    derEy+= meshy_D2[k][lx+x1][ly+y1] * t;
                                }
                            }
                    derEx *= 2 ;
                    derEy *= 2 ;

                    if(keepborder &&  lx==0)
                        x=0;
                    else if (keepborder && lx == nx-1)
                        x=maxx;
                    else
                    {
                        x-=derEx/ 4./derder_E;
                        if(keepborder)
                            x=CLAMP(x,0,maxx);/* AVOID CRISIS */
                        else
                            x=CLAMP(x,-300,maxx+300);/* AVOID CRISIS */
                    }

                    if(keepborder &&  ly==0 )
                        y=0;
                    else if(keepborder && ly == ny-1)
                        y=maxy;
                    else
                    {
                        y-=derEy/ 4./derder_E;
                        if(keepborder)
                            y=CLAMP(y,0,maxy); /* AVOID CRISIS */
                        else
                            y=CLAMP(y,-300,maxy+300); /* AVOID CRISIS */
                    }
                    meshSetNoundo(mesh, lx,ly,x,y);
                    /*  GAUSS_STYLE_UPDATE */
                    for(x1=MAX(0,lx-2); x1 < MIN(lx+2,nx+2) ; x1++)
                        for(y1=MAX(0,ly-2); y1 < MIN(ly+2,ny+2)  ; y1++)
                        {
                            COMPUTE_D2(x1,y1);
                        }

                    partial_change+=sqrt((x-ox)*(x-ox) + (y-oy)*(y-oy));
                }
            }
        if(partial_change < 0.1)
            break;

        change+=partial_change;
    }
    /*********/
#ifdef SELFTEST
    ene=0;
    for(x1=0; x1 < nx+2 ; x1++)
        for(y1=0; y1 < ny+2  ; y1++)
        {
            COMPUTE_D2(x1,y1);
            for (k=0; k<2; k++)
                ene+=(meshx_D2[k][x1][y1]*meshx_D2[k][x1][y1]+
                      meshy_D2[k][x1][y1]*meshy_D2[k][x1][y1]);
        }
    if(ene>oldene)
        printf("ERROR mesh energy has gone from %g to %g \n!",oldene, ene);
#endif
    /********/
#ifdef PRESERVE_VARIANCE
    aft=mesh_variance(mesh, anykind);
    mesh_normalize_variance(mesh,anykind,bef,aft);
#endif
#ifdef RECOMPUTE_DISTANCE
    change=meshDistance(mesh,&copymesh,1);
    meshFreeReally(&copymesh);
#endif

    return change;
}

/*****************************************************************************
CHOOSING INTERFACE
*/

char * smooth_mesh_energy_names[]= {"elastic","thin plate",NULL} ;

double smooth_energy_mesh(int type,MeshT *mesh,int dontoverlap, int keepborder,double orthogonal, double maxx, double maxy)
{
    if(type==0)
        return
            smooth_elastic_mesh(mesh,  			  dontoverlap,
                                keepborder,			  orthogonal,
                                maxx,maxy);
    else   if(type==1)
        return smooth_thin_plate(mesh,keepborder,maxx,maxy);
    else   abort();
}
