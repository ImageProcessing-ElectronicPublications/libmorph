/* my_malloc.c : memory allocation routines with error checking
//

   Written and Copyright (C) 1994-2000 by Michael J. Gourlay

This file is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This file is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this file; see the file LICENSE.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "braindead_msvc.h"

/* Apollo w/ Domain/OS SR10.4.1, BSD 4.3 has no "malloc.h".  Thanks to PW.
//
// Windows NT/95 using the cygwin package already includes "malloc.h".
// Thanks to Geoff Lawler.
*/

#if defined(apollo) || defined(__CYGWIN32__)
#else
#include <stdlib.h>
#endif

#include "my_malloc.h"

/* timestamp: embed a string in the object for later perusal */
static const char *timestamp = "Copyright (C) Michael J. Gourlay " __DATE__ ;

/* NAME
//   listAppend: append a new element to the end of a reallocated list
//
//
// DESCRIPTION
//   A list, implemented as an array, is reallocated to have a new
//   element at the end of the original list.  The contents of the
//   original list is kept intact, but the address of the list elements
//   might change due to the realloc.
//
//   The newly appended list element is left uninitialized.
//
//
// ARGUMENTS
//   root (in/out): address of the variable that points to the
//     beginning of the list.  The list is an array of a type whose
//     size is 'size'.  DANGER:  The contents of 'root' might change
//     due to a realloc().
//
//   nmemb (in/out): address of the variable that holds the number of
//     elements in the list.  The contents 'nmemb' will increment to
//     include the newly appended list element.
//
//   size (in): size of an element of the list.
//
//
// RETURN VALUE
//   If the memory allocation for the new list failes, -2 is returned.
//   Otherwise, the index of the new element (which is the same as the
//   number of elements in the new list, minus one) is returned.
//
//
// NOTES
//   The user is responsible for freeing the memory allocated by
//   listAppend, either by calling listDelete for each element in the
//   list, or an explicit call to free().
//
//   This routine uses realloc().  realloc() is DANGEROUS because it can
//   and will reassign the address of the elements of the array pointed
//   to by 'root'.  That means that if you have something other than
//   'root' pointing to any elements in the 'root' array, and you call
//   listAppend, and the realloc() in listAppend reassigns the addresses,
//   the other pointers will point at nonsense.  Use with care.
//
//   The realloc() in this routine is NOT the mjg_realloc() that does
//   memory checking.  If you create a list with listAppend, then you
//   should not use mjg_free to eliminate the memory.  This is because
//   listAppend is used by mjg_realloc to do internal book-keeping.
//
//
// SEE ALSO
//   listDelete()
*/
int listAppend(void ** root, int *nmemb, const int size)
{
    if((*root = realloc(*root, size * (*nmemb + 1))) == NULL)
    {
        fprintf(stderr, "listAppend: bad alloc: %i\n", *nmemb + 1);
        return -2;
    }

    (*nmemb) ++;
    return *nmemb - 1;
}

/* NAME
//   listDelete: delete an element from a list
//
//
// ARGUMENTS
//   index (in): index of the element to delete
//
//
// RETURN VALUES
//   If 'index' is an invalid value, -1 is returned.
//   If the reallocation fails, -2 is returned.
//   Otherwise, 0 is returned.
//
//
// SEE ALSO
//   listAppend()
*/
int listDelete(void ** root, int *nmemb, const int size, const int index)
{
    int remainder = *nmemb - index - 1;
    if((index < 0) || (index >= *nmemb))
    {
        return -1;
    }

    memmove(((char*)(*root) + index*size), ((char*)(*root) + (index+1)*size),
            remainder * size);

    if(*nmemb > 1)
    {
        if((*root = realloc(*root, size * (*nmemb - 1))) == NULL)
        {
            fprintf(stderr, "listDelete: bad alloc: %i\n", *nmemb - 1);
            return -2;
        }
    }
    else
    {
        memset(*root, 0, size);
    }

    (*nmemb) --;

    return 0;
}

#ifdef DEBUG

/* MEM_BLOCK_: tokens for tagging MemBlocks */
#define MEM_BLOCK_HEAD_ACTIVE   0xfeedfaceabad1deall
#define MEM_BLOCK_HEAD_INACTIVE 0xdeadbeefabadcafell
#define MEM_BLOCK_TAIL_ACTIVE   0xacc01adeabaddeedll
#define MEM_BLOCK_TAIL_INACTIVE 0xcafef00dbeedeca1ll

struct MemBlock
{
    long long *head;    /* Address of the block head */
    void      *user;    /* Address of the beginning of client memory */
    long long *tail;    /* Address of the block tail */
    size_t     size;    /* Size (in bytes) of the user portion of this chunk */
    long       nelem;   /* number of elements */
    int        elsize;  /* element size (in bytes) */
    int        line;    /* line in file of caller that requested the memory  */
    char      *file;    /* file of caller that requested the memory */
};

/* mb_list: list of MemBlocks
// NOTE: Not reentrant.
*/
static struct MemBlock *mb_list = NULL;

/* mb_num: size of the mb_list array
// NOTE: Not reentrant.
*/
static int mb_num = 0;

static int memBlockAppend(void)
{
    return listAppend((void**)&mb_list, &mb_num, sizeof(struct MemBlock));
}

static int memBlockDelete(const int index)
{
    return listDelete((void**)&mb_list, &mb_num, sizeof(struct MemBlock), index);
}

static void memBlockPrint(const struct MemBlock * const self, FILE *stream)
{
    fprintf(stream, "MemBlock user   %p\n", self->user);

    if ( /* CONSTANTCONDITION */ sizeof(size_t) == sizeof(int) )
        fprintf(stream, "MemBlock size   %i %x\n", self->size, self->size);
    else
        fprintf(stream, "MemBlock nelem  %li %lx\n", self->nelem, self->nelem);

    fprintf(stream, "MemBlock elsize %i %x\n", self->elsize, self->elsize);
    fprintf(stream, "MemBlock file '%s' line %i\n", self->file, self->line);
    fprintf(stream, "MemBlock head   %p\n", self->head);
    fprintf(stream, "MemBlock tail   %p\n", self->tail);
    if(self->head != NULL)
    {
        fprintf(stream, "MemBlock head[0] %llx ", self->head[0]);

        if(self->head[0] != MEM_BLOCK_HEAD_ACTIVE)
        {
            fprintf(stream, "CORRUPT");
        }
        fprintf(stream, "\n");

        if(self->tail != NULL)
        {
            fprintf(stream, "MemBlock tail[0] %llx ", self->tail[0]);
            if(self->tail[0] != MEM_BLOCK_TAIL_ACTIVE)
            {
                fprintf(stream, "CORRUPT");
            }
            fprintf(stream, "\n");
        }
    }
}

/* NAME
//   memBlockCompare: compare function for qsort
//
// ARGUMENTS
//   p1, p2 (in): pointers to MemBlocks
//
// DESCRIPTION
//   The qsort function needs a compare function for comparing elements
//   in an array.  memBlockCompare performs the comparison by using the
//   "user" field of the MemBlock struct.  The idea is that if some
//   chunk of memory is corrupt, then it will be useful to know what is
//   stored in adjacent memory areas.
//
// NOTES
//   One problem with sorting according to address is that the order
//   that the MemBlock appears in the mb_list array of MemBlocks will
//   be lost.  That information might be useful to determine roughly
//   when the memory was allocated.
//
// SEE ALSO
//   memBlockSort(), qsort()
*/
static int memBlockCompare(const void *p1, const void *p2)
{
    const struct MemBlock *mb1 = p1;
    const struct MemBlock *mb2 = p2;
    const long             u1  = (long) mb1->user; /* cast */
    const long             u2  = (long) mb2->user; /* cast */
    if(u1 > u2) return 1;
    else if(u1 < u2) return -1;
    return 0;
}

/*
// SEE ALSO
//   memBlockCompare()
*/
static void
memBlockSort(void)
{
    fprintf(stderr, "\n ---- memBlockSort: sorting by address\n");
    qsort(mb_list, mb_num, sizeof(struct MemBlock), memBlockCompare);
}

/* NAME
//   memBlockInventory: display list of MemBlocks
//
//
// ARGUMENTS
//   all(in): flag indicating whether all MemBlocks should be
//     diplayed, or only the active ones.
*/
void memBlockInventory(const int all)
{
    int mbi;

    fprintf(stderr, "memBlockInventory: %i ", all);
    if(all)
    {
        fprintf(stderr, "(reporting all %i blocks)\n", mb_num);
    }
    else
    {
        fprintf(stderr, "(reporting %i unfreed blocks)\n", mb_num);
    }

    for(mbi=0; mbi < mb_num; mbi++)
    {
        if(all || mb_list[mbi].head != NULL)
        {
            fprintf(stderr, "MemBlock %i:\n", mbi);
            memBlockPrint(&mb_list[mbi], stderr);
            fprintf(stderr, "\n");
        }
    }
    fprintf(stderr, "\n");
}

/* NAME
//   memBlockCheck: check MemBlock for corruption
//
//
// DESCRIPTION
//   The memory in a MemBlock has tags at the beginning and end of the
//   allocated memory chunk.  If the tags do not have one of a small
//   set of valid values then the memory chunk is considered to have
//   been corrupted.  This sort of rudimentary checking will discover
//   most cases of exceeding array boundaries.
*/
static int memBlockCheck(const struct MemBlock * const self)
{
    if(self->head != NULL)
    {
        if(   (self->head[0] != MEM_BLOCK_HEAD_ACTIVE)
                && (self->head[0] != MEM_BLOCK_HEAD_INACTIVE))
        {
            fprintf(stderr, "memBlockCheck: corrupt head\n");
            memBlockPrint(self, stderr);
            fflush(stdout);
            fflush(stderr);
#if DEBUG >= 1
            abort();
#else
            return 1;
#endif
        }
        if(self->tail != NULL)
        {
            if(   (self->tail[0] != MEM_BLOCK_TAIL_ACTIVE)
                    && (self->tail[0] != MEM_BLOCK_TAIL_INACTIVE))
            {
                fprintf(stderr, "memBlockCheck: corrupt tail\n");
                memBlockPrint(self, stderr);
                fflush(stdout);
                fflush(stderr);
#if DEBUG >= 1
                abort();
#else
                return 1;
#endif
            }
        }
    }
    return 0;
}

int memBlockCheckAll(void)
{
    int mbi;
    int rv = 0;
    for(mbi=0; mbi < mb_num; mbi++)
    {
        rv += memBlockCheck(&mb_list[mbi]);
    }
    return rv;
}

/* NAME
//   memBlockIndex: Search for pointer in the list of MemBlocks
//
//
// NOTES
//   Search starting from the end of the list of MemBlocks.
//   This is because it is possible (perhaps likely) that the same
//   memory address will be used multiple times between malloc/free
//   calls, and we want to reference the most recently malloced block
//   with the given address.
*/
static int memBlockIndex(const void * const ptr)
{
    int mbi;

    for(mbi=mb_num-1; mbi >= 0; mbi--)
    {
        if(ptr == mb_list[mbi].user)
        {
            /* Found pointer in memory table */
            return mbi;
        }
    }
    return -1;
}

static int mem_exit_check_registered = 0;

static void memExitCheck(void)
{
#if DEBUG >= 1
    fprintf(stderr, "\n ==== memExitCheck: %s\n", timestamp);
#if VERBOSE >= 2
    memBlockInventory(1);
#if 0
    memBlockSort();
    memBlockInventory(1);
#endif
#else
    memBlockInventory(0);
#if 0
    memBlockSort();
    memBlockInventory(0);
#endif
#endif
#endif
}

/* TAG_SIZE: size, in bytes, of the head and tail tags
//
// NOTES
//   TAG_SIZE must also be the size of a memory block alignment for
//   heap allocation.  Usually, this is the same as the size of a word
//   for the architeture, but can be the size of a double word for a
//   64-bit architecture.  Usually, a "long long" here will suffice.
*/
#define TAG_SIZE sizeof(long long)

#else
#define TAG_SIZE 0
#endif /* DEBUG */

/* NAME
//   mjg_realloc: memory allocation with debugging
//
//
// ARGUMENTS
//   ptr (in/out): address of the user portion of previously allocated memory
//
//   nelem (in): number of elements to allocate
//
//   elsize (in): size of the element, in bytes
//
//   file (in): name of the file from which mjg_realloc was called
//
//   line (in): line number of the file from which mjg_realloc was called
//
//
// DESCRIPTION
//   mjg_realloc is the workhorse routine for several incarnations of
//   this memory allocation package.  Usually, mjg_realloc would not be
//   called by the client directly, but instead would be called through
//   one of several calling macros, such as REALLOC, MY_CALLOC or
//   CALLOC, which provide programming interfaces more like the system
//   alloc routines.
//
//   mjg_realloc sets up and performs memory allocation debugging table
//   entries which can be used to help find array bounds violations and
//   un-freed memory blocks ("memory leaks").
//
//
// NOTES
//   The address "ptr" is searched for in the list of MemBlocks.  If it is
//   found, and if the associated MemBlock is active, then the head of that
//   MemBlock is used as the actual beginning of the memory chunk.  (Note
//   that it is possible for "ptr" to be found in the list of MemBlocks and
//   for that MemBlock to be inactive.  This probably means that the client,
//   at one time, used mjg_realloc to allocate a memory chunk, freed it using
//   mjg_free, then used a system alloc routine which coincidentally created
//   another memory chunk which happened to start at the same address as
//   "ptr".)
//
//   If "ptr" is not found in the list of MemBlocks or if "ptr" is found but
//   the MemBlock is inactive, then "ptr" is used as the actual beginning of
//   the memory chunk.  In this situation, it is probably the case that the
//   client used the system alloc routines to allocate the memory chunk, and
//   it using mjg_realloc instead of realloc.  I.e., the client is mixing use
//   of mjg_realloc and system alloc routines, which is a Bad Idea.
//
//
// SEE ALSO
//   macros: MY_CALLOC, REALLOC, CALLOC, MALLOC, FREE, STRDUP
*/
void * mjg_realloc(void * const ptr, const long nelem, const int elsize, const char * const file, const int line)
{
    void *mem = ptr;

#ifdef DEBUG
    int  mbi = memBlockIndex(ptr);   /* Find MemBlock for this chunk */

    if(!  mem_exit_check_registered)
    {
        /* Register memory block checker to be called on exit */
        atexit(memExitCheck);
        mem_exit_check_registered = 1;
    }
    fflush(stdout);
    fflush(stderr);
#endif

#if VERBOSE >= 2
    fprintf(stderr, "mjg_realloc: %p %li %i %s %i\n",
            ptr, nelem, elsize, file, line);
#endif

#ifdef DEBUG
#ifdef SUNOS
    malloc_verify();
#endif

#if defined(sgi)
    /* If this is not resolved at link time, try linking with -lmalloc */
    mallopt(M_DEBUG, 1);
#endif

    if(memBlockCheckAll())
    {
        fprintf(stderr, "mjg_realloc: ERROR: memBlockCheckAll 1 found errors\n");
    }

    /* See if MemBlock was found for this chunk */
    if(mbi >= 0)
    {
        /* A MemBlock was found for the chunk with user portion at ptr */
        if(    (mb_list[mbi].head != NULL)
                && (mb_list[mbi].head[0] == MEM_BLOCK_HEAD_ACTIVE))
        {
            /* The MemBlock is active so its head is the top of the memory chunk */
            mem = mb_list[mbi].head;
        }
    }
#endif /* DEBUG */


    /* Check the validity of the input parameters */
    if(nelem < 0)
    {
        fprintf(stderr, "mjg_realloc: %s: %i: ERROR: Bad Value: nelem=%li\n",
                file, line, nelem);

#if DEBUG >= 1
        fflush(stderr);
        fflush(stdout);
        abort();
#endif

        return NULL;
    }
    if(elsize < 0)
    {
        fprintf(stderr, "mjg_realloc: %s: %i: ERROR: Bad Value: elsize=%i\n",
                file, line, elsize);

#if DEBUG >= 1
        fflush(stderr);
        fflush(stdout);
        abort();
#endif

        return NULL;
    }

    if(nelem * elsize <= 0)
    {
        fprintf(stderr,
                "mjg_realloc: %s %i: WARNING: allocating no memory\n", file, line);
        fprintf(stderr, "mjg_realloc: %s %i: WARNING: nelem=%li elsize=%i\n",
                file, line, nelem, elsize);
        fprintf(stderr, "mjg_realloc: %s %i: WARNING: product=%li\n",
                file, line, nelem * elsize);
#if DEBUG >= 1
        fprintf(stderr, "mjg_realloc: ready to abort.  press return.\n");
        getchar();
        abort();
#endif
    }


    /* Allocate the client memory:
    // The extra 2 TAG_SIZE elements are for the head and tail tags.
    // One of the extra TAG_SIZE is for aligment.
    */
    if((mem=realloc(mem, (nelem*elsize)+(3 * TAG_SIZE)))==NULL)
    {
        fprintf(stderr, "mjg_realloc: %s %i: Bad Alloc: %li x %i = %li\n",
                file, line, nelem, elsize, nelem * elsize);
    }

#ifdef DEBUG
    else
    {
        /* Assign the various portions of the memory chunk */
        long long  *head = mem;
        char       *user = (char *)(&head[1]);
        long long  *tail = (long long*)(&user[nelem * elsize]);

        /* Refer mem to the user portion of the memory chunk */
        mem = user;

        /* Avoid a bus error by finding an address with proper alignment
           for the user and tail.  This is a big hack and makes some
           assumptions about word allignment requirements of a machine --
           specifically that the word size is smaller than or equal to a
           "long long integer", and that alignment is an even multiple of
           the size of a long long.  If this causes problems for your
           architecture, then turn memory debugging off, or comment out the
           tail reference lines throughout this module.  Also, report the
           error to the authors.

        */

        {
            unsigned long align = (long)tail;
            align = ((align + (sizeof(long long)-1)) / sizeof(long long)) * sizeof(long long);

#if (VERBOSE >= 2)
            fprintf(stderr, "tail was=%p  align=%li\n", tail, align);
#endif
            tail = (long long *)align;

#if (VERBOSE >= 2)
            fprintf(stderr, "tail is=%p\n", tail);
#endif

        }

        /* Tag the memory chunk */
        head[0] = MEM_BLOCK_HEAD_ACTIVE;
        tail[0] = MEM_BLOCK_TAIL_ACTIVE;

        /* Clear new portion of the memory chunk */
        if((ptr == NULL) || (mbi >= 0))
        {
            /* By default, clear the entire memory chunk */
            char *new        = mem;
            size_t  new_size = nelem * elsize;

            /* If this is really a realloc, only clear new portion */
            if(mbi >= 0)
            {
                size_t old_size = mb_list[mbi].size;
                new             = &user[old_size];
                new_size        = nelem * elsize - old_size;
            }

#if (DEBUG >= 3)
            fprintf(stderr, "mjg_realloc: %s: %i: setting %i bytes at %p\n",
                    file, line, new_size, new);
            memset(new, 1, new_size);
#else
            memset(new, 0, new_size);
#endif
        }

        /* If there was no MemBlock associated with this chunk, try to make one */
        if(mbi < 0)
        {
            mbi = memBlockAppend();
        }

        if(mbi >= 0)
        {
            /* Assign the MemBlock members */
            mb_list[mbi].head = head;
            mb_list[mbi].user = user;
            mb_list[mbi].tail = tail;
            mb_list[mbi].size = nelem*elsize;
            mb_list[mbi].nelem = nelem;
            mb_list[mbi].elsize = elsize;
            mb_list[mbi].line = line;
            mb_list[mbi].file = strdup(file);
            if(mb_list[mbi].file == NULL)
            {
                fprintf(stderr,"mjg_realloc: %s: %i: bad interal alloc: file\n",file,line);
            }
        }
    }

#ifdef SUNOS
    malloc_verify();
#endif

    if(memBlockCheckAll())
    {
        fprintf(stderr, "mjg_realloc: ERROR: memBlockCheckAll 2 found errors\n");
    }

#endif /* DEBUG */


#if (VERBOSE >= 2)
    fprintf(stderr,
            "mjg_realloc:%s:%i:allocated %li at %p\n", file, line, nelem*elsize, mem);
#endif

    return mem;
}

/* NAME
//   mjg_free: heap memory deallocation with error checking
//
//
// ARGUMENTS
//   ptr (in/out): address of the user data area
//
//   file (in): name of the file from which mjg_realloc was called
//
//   line (in): line number of the file from which mjg_realloc was called
//
//
// SEE ALSO
//   mjg_realloc(), FREE() macro
*/
/*ARGSUSED*/
void mjg_free(void * const ptr, const char * const file, const int line)
{
#if VERBOSE >= 2
    fprintf(stderr, "mjg_free: %s: %i: %p\n", file, line, ptr);
    fflush(stderr);
#endif

#ifdef DEBUG
#ifdef SUNOS
    malloc_verify();
#endif

    if(memBlockCheckAll())
    {
        fprintf(stderr, "mjg_free: ERROR: memBlockCheckAll found errors\n");
    }

    {
        int mbi;

        if((mbi = memBlockIndex(ptr)) >= 0)
        {

            /* Check to see whether this block is okay */
            if(NULL == mb_list[mbi].head)
            {
                fprintf(stderr, "mjg_free: %s: %i: empty block\n", file, line);
                return;
            }
            else
            {
                if(   (MEM_BLOCK_HEAD_INACTIVE == mb_list[mbi].head[0])
                        && (MEM_BLOCK_TAIL_INACTIVE == mb_list[mbi].tail[0]))
                {
                    fprintf(stderr, "mjg_free: block already freed?\n");
                }
            }

#if VERBOSE >= 3
            printb("mjg_free: %s: %i: found ptr at index %i\n", file, line, mbi);
            memBlockPrint(&mb_list[mbi], stderr);
#endif

            /* Fill the old memory chunk */
            memset(ptr, 0xfe, mb_list[mbi].size);

            /* Tag this block as inactive */
            mb_list[mbi].head[0] = MEM_BLOCK_HEAD_INACTIVE;
            mb_list[mbi].tail[0] = MEM_BLOCK_TAIL_INACTIVE;
        }

        if(mbi < 0)
        {
            /* Did not find pointer in the list of MemBlocks */
            fprintf(stderr, "mjg_free: WARNING: %s: %i: freeing unknown pointer %p\n",
                    file, line, ptr);

#if DEBUG >= 1
            fflush(stderr);
            fflush(stdout);
            abort();
#endif

            free(ptr);

        }
        else
        {
            free(mb_list[mbi].head);
            mb_list[mbi].head = NULL;

#if DEBUG <= 1
            /* Note that a DEBUG level of greater than 1 will result in a
            // record of every allocation done through these routines to
            // be kept, which could result in extremely slow performance.
            */
            memBlockDelete(mbi);
#endif
        }
    }

#ifdef SUNOS
    malloc_verify();
#endif
#else
    free(ptr);

#endif /* DEBUG */

    return;
}

/* NAME
//   mjg_strdup: strdup with memory checking
//
//
// SEE ALSO
//   mjg_realloc(), STRDUP() macro
*/
char *
mjg_strdup(const char *s, const char * const file, const int line)
{
    if(NULL == s)
    {
        return NULL;
    }

    {
        char *dup = mjg_realloc(NULL,(long)(strlen(s) + 1), sizeof(char),file,line);
        if((dup != NULL) && (s != NULL))
        {
            strcpy(dup, s);
        }
        else
        {
            fprintf(stderr, "mjg_strdup: ERROR: mjg_realloc failed\n");
        }
        return dup;
    }
}

#ifdef TEST_MY_MALLOC

void test1(void)
{
#ifdef DEBUG
    int *ip = MY_CALLOC(3, int);
    ip[0] = 1;
    ip[1] = 2;
    ip[2] = 3;
    printf("ip = %i %i %i ?= 16843009\n", ip[0], ip[1], ip[2]);
    FREE(ip);

    printf("\nfreeing twice; should err: unknown pointer or empty block\n");
    FREE(ip);
#endif
}

void test2(void)
{
#ifdef DEBUG
    int *ip = MY_CALLOC(3, int);
    ip[-1] = -1;  /* corrupt the head */
    ip[0] = 1;
    ip[1] = 2;
    ip[2] = 3;
    printf("should err:    corrupt head\n");
    FREE(ip);
#endif
}

void test3(void)
{
#ifdef DEBUG
    int *ip = MY_CALLOC(3, int);
    printf("MY_CALLOC and FREE: should be ok\n");
    ip[0] = 1;
    ip[1] = 2;
    ip[2] = 3;
    FREE(ip);
#endif
}

void test4(void)
{
#ifdef DEBUG
    int *ip = calloc(3, sizeof(int));

    printf("Mixing system calloc and mjg_realloc\n");
    ip[0] = 1;
    ip[1] = 2;
    ip[2] = 3;

    ip = REALLOC(ip, sizeof(int)*5);
    memBlockPrint(&mb_list[memBlockIndex(ip)], stderr);
    ip[3] = 4;
    ip[4] = 5;
    /* Should be okay */
    FREE(ip);
#endif
}

void test5(void)
{
#ifdef DEBUG
    int *ip = MY_CALLOC(3, int);
    ip[0] = 1;
    ip[1] = 2;
    ip[2] = 3;
    ip[3] = 4;  /* corrupt tail */
    ip[4] = 4;  /* corrupt tail */
    printf("should err:    corrupt tail\n");
    FREE(ip);
#endif
}

void test6(void)
{
#ifdef DEBUG
    int *ip = MY_CALLOC(3, int);
    ip = MY_CALLOC(3, int);
    FREE(ip);
    ip = MY_CALLOC(3, int);
    ip = MY_CALLOC(3, int);
    ip[0] = 1;
    ip[1] = 2;
    ip[2] = 3;
    printf("Leaving block without freeing memory\n");
    printf("should create warning on exit\n");
#endif
}

void test7(void)
{
    float *fp = MY_CALLOC(3, float);
    fp[0] = 1.0;
    fp[1] = 2.0;
    fp[2] = 3.0;
    FREE(fp);

    printf("\nfreeing twice: should err: unknown poiner or empty block\n");
    FREE(fp);
}

void test8(void)
{
#ifdef DEBUG
    {
        float *fp;

        fp = MY_CALLOC(3, float);
        fp[-1] = -1;  /* corrupt the head */
        fp[0] = 1.0;
        fp[1] = 2.0;
        fp[2] = 3.0;
        printf("should err:    corrupt head\n");
        FREE(fp);

        printf("\n-=-\n");
        fp = MY_CALLOC(3, float);
        fp[0] = 1.0;
        fp[1] = 2.0;
        fp[2] = 3.0;
        printf("-=-\n");

        /* Test realloc */
        memBlockPrint(&mb_list[memBlockIndex(fp)], stderr);
        fp = REALLOC(fp, sizeof(float)*5);
        printf("---\n");
        memBlockPrint(&mb_list[memBlockIndex(fp)], stderr);
        fp[3] = 4.0;
        fp[4] = 5.0;
        /* Should be okay */
        FREE(fp);

        printf("\n");
        fp = MY_CALLOC(3, float);
        fp[0] = 1.0;
        fp[1] = 2.0;
        fp[2] = 3.0;
        fp[3] = 4.0;  /* corrupt tail */
        printf("should err:    corrupt tail\n");
        FREE(fp);

        printf("\n");
        fp = MY_CALLOC(3, float);
        fp[0] = 1.0;
        fp[1] = 2.0;
        fp[2] = 3.0;
        fp[3] = 4.0;  /* corrupt tail */
        /* Leaving block without freeing memory -- should create warning */
    }
#endif /* DEBUG */
}

int main(int argc, char **argv)
{
    int test_index;
    if(argc != 2)
    {
        fprintf(stderr, "usage: %s test_index\n", argv[0]);
        return 1;
    }
    test_index = atoi(argv[1]);

    if(1 == test_index) test1();
    if(2 == test_index) test2();
    if(3 == test_index) test3();
    if(4 == test_index) test4();
    if(5 == test_index) test5();
    if(6 == test_index) test6();
    if(7 == test_index) test7();
    if(8 == test_index) test8();

#ifdef DEBUG
#if VERBOSE >= 2
    printf("\n\n\n\n------------");
    memBlockInventory(1);
    printf("------------\n\n\n\n");
#endif

    printf("\n\n\n\n========\n");
    memBlockInventory(0);
#endif

    return 0;
}
#endif /* TEST_MY_MALLOC */
