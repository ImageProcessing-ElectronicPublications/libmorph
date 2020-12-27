#ifdef FUN
/* this code is not compiled as such, but rather included in other code */


/*TABLE HYPER  PRECISION
adds TYH bits of precision to table indexes without needing a
table that big, and without overusing 'double' variables;
does not add any visible difference, but has a small computational cost
*/
#if (SINC_TABLE_UNIT*KERNEL_WIDTH) < (1<<11)
#define TYH 4
#define SINC_TABLE(I) sinc_table[ (I) >> TYH]
#elif (SINC_TABLE_UNIT*KERNEL_WIDTH) < (1<<13)
#define TYH 2
#define SINC_TABLE(I) sinc_table[ (I) >> TYH]
#else
#define TYH 0
#define SINC_TABLE(I) sinc_table[I]
#endif

#if defined(KERNEL_sinc_fast) && ( SINC_TABLE_UNIT  < (PIXEL_MAX-PIXEL_MIN) )
#warning the kernel is based on the sinc(); the function sinc is replaced by a table; but the table is not precise enough for your type of pixels
/* in this case you may want to interpolate in the table
#define MASK ((1<<TYH)-1)
#define SINC_TABLE(I) (sinc_table[ (I) >> TYH]     * ((I)&MASK) + \
                       sinc_table[ 1+( (I) >> TYH)]* ((1<<TYH)-   ((I)&MASK)) )
but it is unclear if this is really any faster than using the sinc()
function with doubles
 */
#endif

static void  FUN(KERNEL)     /*that is, resample_array_inv_conv_ ## KERNEL */
(const double *F,
 const PIXEL_TYPE *src, int s_len, int s_stride,
 PIXEL_TYPE *dst, int d_len, int d_stride)
{
    int i,p=0,j,pj, firstj,lastj;
    double x,px,c,s,v,dx;
#ifdef KERNEL_sinc_fast
    int index, indexbound, increment;
#endif
    px=F[1];
    for(i=0; i<d_len; i++)
    {
        v=0;
        c=0;
        x=F[i];
        /* FIXED! READ OUT OF ARRAY!!!
        //nx=F[i+1]; */
        dx=ABS(x-px);
        if(dx<1) dx=1;

        firstj=floor(x-dx*KERNEL_WIDTH);
        lastj=ceil(x+dx*KERNEL_WIDTH);

        j=firstj;

        /* note that in the past I had  reversed the usage of the filter;
           this does not really make any difference as long as the filter is
           symmetric... but I want to be more precise     */

#ifdef KERNEL_sinc_fast
        index=((  (((double)firstj-x) / dx)  )* (double)(SINC_TABLE_UNIT << TYH));
        increment= ( (SINC_TABLE_UNIT << TYH) / dx  );
        indexbound=( (KERNEL_WIDTH*SINC_TABLE_UNIT) << TYH);
        /*roughly
               index ~ -indexbound;
          but if it is approximated to that, then the warping loses
          sub-pixel precision, and there are visible artifacts
        */
        /* this condition is always true
        //if (index >=  -indexbound) */
        {
            j++;
            index += increment;
        }
#endif

#ifndef KERNEL_sinc_fast
        /* this is the convolution with a generic kernel */
        for ( ; j<=lastj  ; j++ )
        {
            s=KERNEL( ((double)j-x) / dx );
            pj=j;
            if ( pj<0 ) pj=0;
            else if( pj>=s_len) pj=s_len-1;
            v+=s* (double)src[pj* s_stride];
            c+=s;
        }
#else
        /* this is how it is computed very fast for a kernel based on sinc, such as lanczos */
        for ( ;  index<=0 ; )
        {
            s=SINC_TABLE(-index );
            pj=j;
            if ( pj<0 ) pj=0;
            else if( pj>=s_len) pj=s_len-1;
            v+=s* (double)src[pj*s_stride];
            c+=s;
            j++;
            index+=increment;
        }
        for ( ;  index <= indexbound  ; )
        {
            s=SINC_TABLE(index);
            pj=j;
            if ( pj<0 ) pj=0;
            else if( pj>=s_len) pj=s_len-1;
            v+=s* (double)src[pj*s_stride];
            c+=s;
            j++;
            index+=increment;
        }
#endif

        if(c<0.0001 && c > -0.0001)
            fprintf(stderr,"%s:%s:%d: too small c=%f\n",__FILE__,__FUNCTION__,__LINE__,c);
        else
            v/=c;
        dst[p]=CLAMP(v,PIXEL_MIN,PIXEL_MAX);

        p+= d_stride;
        px=x;
    }
}

#endif
