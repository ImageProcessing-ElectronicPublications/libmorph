
#ifndef _RESAMPLE_INCLUDED_
#define _RESAMPLE_INCLUDED_
#include "warp.h"

extern void (*resample_array_inv)(const double *F,
                                  const PIXEL_TYPE *src, int s_len, int s_stride,
                                  PIXEL_TYPE *dst, int d_len, int d_stride);

/* choice of kernel by name */
extern char *resample_array_inv_names[]; /* is null terminated */
void mesh_resample_choose_aa(int f);
void mesh_resample_choose_aa_by_name(char *s);

/** backward compatible interface, for older code
 *
 */

static inline void resample_array_inv_bc
(const double *F,
 const PIXEL_TYPE *src,
 PIXEL_TYPE *dst, int len, int stride)
{

    resample_array_inv(F,src,len,stride,dst,len,stride);
}

#endif
