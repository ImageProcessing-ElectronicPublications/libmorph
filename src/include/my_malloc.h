/* my_malloc.h: memory allocation routines with error checking
//
// Written and Copyright (C) 1994-1999 by Michael J. Gourlay
//
// This file is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this file; see the file LICENSE.  If not, write to
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

If you want to use my memory allocation routines,

() Create symbolic links from
   ~gourlay/src/lib/my_malloc.c and
   ~gourlay/src/lib/my_malloc.h
   to the directory where your code is.

() Include "my_malloc.h" in each module where you want to use those
   routines.

   Also, define the symbol "DEBUG" /before/ including "my_malloc.h".
   The best way to do this is /not/ to add a #define DEBUG statement,
   but instead to add the flag -DDEBUG to the CFLAGS variable in the
   Makefile.  Also, when compiling my_malloc.c, you should also define
   DEBUG.

() In place of each allocation routine, such as malloc, calloc, realloc,
   or free, use the same arguments, but capitalize the name of the
   routine.  E.g. use MALLOC, CALLOC, REALLOC, or FREE.

   It is not possible to mix stdlib allocation routines with mine.  I.e.
   if you allocate with "malloc", you must free with "free", and if you
   allocate with MALLOC, you must free with FREE.

() Remember to include my_malloc.o in the list of objects you link to
   for your program.

() For examples of using my_malloc have a look at the code in
   ~gourlay/src/graph/xmorph or ~gourlay/src/math/lattice.
   The Makefiles in those directories might also be of interest.

*/

#ifndef _MY_MALLOC_H
#define _MY_MALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

    /* GNU C header file for string.h is broken in some versions.  */
#if ( defined(__GNUC__) && defined (STRDUP_PROTO_BROKEN) )
    extern char *strdup (const char *s);
#endif

    int listAppend(void ** root, int *nmemb, const int size);
    int listDelete(void ** root, int *nmemb, const int size, const int index_) ;

#ifdef DEBUG
    void memBlockInventory(const int all) ;
    int memBlockCheckAll(void) ;
#endif

    void * mjg_realloc(void * const ptr, const long nelem, const int elsize, const char * const file, const int line);

    void mjg_free(void * const ptr, const char * const file, const int line);

    char * mjg_strdup(const char *s, const char * const file, const int line);

#ifdef DEBUG
#define CALLOC(nelem,elsize)  mjg_realloc(NULL,nelem,elsize,__FILE__,__LINE__)
#define MALLOC(size)          mjg_realloc(NULL,size,1,__FILE__,__LINE__)
#define MY_CALLOC(nelem,type) mjg_realloc(NULL,nelem,sizeof(type),__FILE__,__LINE__)
#define REALLOC(ptr,size)     mjg_realloc(ptr,(long)(size),1,__FILE__,__LINE__)
#define STRDUP(s)             mjg_strdup(s,__FILE__,__LINE__)
#define FREE(ptr)             mjg_free(ptr,__FILE__,__LINE__)
#else
#define CALLOC                calloc
#define MALLOC                malloc
#define MY_CALLOC(nelem,type) calloc(nelem,sizeof(type))
#define REALLOC               realloc
#define STRDUP                strdup
#define FREE                  free
#endif

#define ALLOCA alloca

#ifdef __cplusplus
}
#endif

#endif
