/* tga.c: Targa TrueVision image file handling routines
//
// Written and Copyright (C) 1994-1999 by Michael J. Gourlay
//
// Provided as is.  No warrantees, express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "my_malloc.h"
#include "tga.h"

/* Global Targa colormap */
static unsigned char tga_cmap_r[16384];
static unsigned char tga_cmap_g[16384];
static unsigned char tga_cmap_b[16384];
static unsigned char tga_cmap_a[16384];

RgbaImageT tga_cmap =
{
    0, 0, 0, 0, 0, 0,
    tga_cmap_r, tga_cmap_g, tga_cmap_b, tga_cmap_a,
};

#define CURR_ROW(irow) ((tgaP->origin_bit)?(irow):(imgP->nrows-(irow)-1))

#define ROW_INC \
{ \
  row_count++; \
  if(tgaP->interleave==TGA_IL_Four) { ilace_row += 4 ; } \
  else if(tgaP->interleave==TGA_IL_Two) { ilace_row += 2 ; } \
  else  { ilace_row ++ ; } \
  if(ilace_row > imgP->nrows) ilace_row = ++ top_row; \
  c_row = CURR_ROW(ilace_row); \
}

/* get_byte : get from fio a byte
*/
#define GET_BYTE(byte, fio) \
{\
  int rv;\
  (byte)=(unsigned char)(rv=getc(fio));\
  if(rv==EOF) {\
    fprintf(stderr, "get_byte: EOF/read error\n");\
    return EOF ;\
  }\
}

/* NAME
//   put_le_word : put into fio a 2-byte little-endian unsigned integer
*/
int put_le_word(short le_int, FILE *fio)
{
    unsigned char b1, b2;

    b1 = le_int & 0xff;

    b2 = (le_int >> 8) & 0xff;

    /* least significant byte comes first */
    if(putc(b1, fio)==EOF) return EOF ;

    /* most significant byte comes last */
    if(putc(b2, fio)==EOF) return EOF ;

    return 0 ;
}

/* NAME
//   get_le_word : get from fio a 2-byte little-endian unsigned integer
*/
long get_le_word(FILE * fio)
{
    unsigned char b1, b2;

    /* least significant byte comes first */
    GET_BYTE(b1, fio);

    /* most significant byte comes last */
    GET_BYTE(b2, fio);

    return (b1 + b2*256);
}

/* NAME
//   get_block : get from fio an block of n bytes, and store it in buf
//
//
// RETURN VALUES
//   return EOF if there is a read error, or 0 otherwise
//
//
// NOTES
//   get_block is nothing but fread with error reporting.
//   Calls to get_block should probably just be replaced
//   with fread.
*/
short get_block(FILE * fio, char *buf, long n)
{
    int rv;

    if(feof(fio)) return EOF ;
    rv=fread(buf, 1, (size_t)n, fio);
    if(rv!=n)
    {
        if(rv)
        {
            fprintf(stderr, "get_block: EOF/read error reading byte %i/%li\n", rv, n);
        }
        return EOF ;
    }

    return 0 ;
}

/* NAME
//   tgaPixelRead: read a Targa pixel from fio into imgP
//
//
// ARGUMENTS
//   npixels: the number of literal pixels to read.
//   size: the size of the pixel in the file, in bits.
//   mapped: tells whether bits are gray, coded RGB, or cmap index.
//
//
// NOTES
//   This code is optimized for speed, not readability or compactness
//   so the redundancy is intentional.
*/
static int tgaPixelRead(FILE *fio, RgbaImageT *imgP, int npixels, int size, int mapped)
{
    int   pcount;

    switch(size)
    {
    case 8:
        if(get_block(fio, (char*)imgP->ri, npixels))
        {
            fprintf(stderr, "tgaPixelRead: read error\n");
            return -1 ;
        }
        if(mapped)
        {
            for(pcount=0; pcount < npixels; pcount++)
            {
                /* cmap indices are stored in ri, so do lookups with r last  */
                imgP->bi[pcount] = tga_cmap.bi[imgP->ri[pcount]];
                imgP->gi[pcount] = tga_cmap.gi[imgP->ri[pcount]];
                imgP->ri[pcount] = tga_cmap.ri[imgP->ri[pcount]];
            }
        }
        else
        {
            memcpy(imgP->gi, imgP->ri, npixels);
            memcpy(imgP->bi, imgP->ri, npixels);
        }
        memset(imgP->ai, RGBA_IMAGE_OPAQUE, npixels);
        break;

    case 16:
    case 15:
    {
        int ip, jp;
        int pixel;

        for(pcount=0; pcount < npixels; pcount++)
        {
            GET_BYTE(ip, fio);
            GET_BYTE(jp, fio);
            if(mapped)
            {
                pixel = ((unsigned int) jp << 8) + ip;
                imgP->ri[pcount] = tga_cmap.ri[pixel];
                imgP->gi[pcount] = tga_cmap.gi[pixel];
                imgP->bi[pcount] = tga_cmap.bi[pixel];
            }
            else
            {
                /* Unpack color bits (5 each for red, green, blue */
                imgP->ri[pcount] = (jp & 0x7c) >> 2;
                imgP->gi[pcount] = ((jp & 0x03) << 3) + ((ip & 0xe0) >> 5);
                imgP->bi[pcount] = ip & 0x1f;
            }
            imgP->ai[pcount] = RGBA_IMAGE_OPAQUE;
        }
    }
    break;

    case 32:
    case 24:
    {
        for(pcount=0; pcount < npixels; pcount++)
        {
            GET_BYTE(imgP->bi[pcount], fio);
            GET_BYTE(imgP->gi[pcount], fio);
            GET_BYTE(imgP->ri[pcount], fio);
            if(size == 32)
            {
                GET_BYTE(imgP->ai[pcount], fio);
            }
            else
            {
                imgP->ai[pcount] = RGBA_IMAGE_OPAQUE;
            }
        }
    }
    break;

    default:
        fprintf(stderr, "tgaPixelRead: unknown pixel size %i\n", size);
        return -1 ;

        /*NOTREACHED*/
        break;

    }
    return 0 ;
}

/* NAME
//   tgaRead: Load a Targa image file from fio into imgP
*/
int tgaRead(tga_hdr_t *tgaP, RgbaImageT *imgP, FILE *fio)
{
    int        col;
    int        rle_count;      /* run-length of data */
    int        c_row;          /* current row index being loaded */
    int        rl_encoded;     /* boolean flag */
    int        row_count;      /* total count of rows loaded */
    int        ilace_row;      /* interlaced row counter */
    int        top_row;        /* where to start over for interlaced images */
    RgbaImageT timg;

    if(tgaP->img_type == TGA_RLE_Map || tgaP->img_type == TGA_RLE_RGB ||
            tgaP->img_type == TGA_RLE_Mono)
    {
        rl_encoded = 1;
    }
    else
    {
        rl_encoded = 0;
    }

    row_count = ilace_row = top_row = 0;
    c_row = CURR_ROW(ilace_row);

    if(rl_encoded)
    {
        int           nbytes;
        int           rle_compressed; /* whether run is encoded or literal */
        int           blir;           /* bytes left in row */
        unsigned char ibyte;
        unsigned char tri;
        unsigned char tgi;
        unsigned char tbi;
        unsigned char tai;

        /* Load the temporary image with info from imgP */
        timg = *imgP;

        while(row_count < imgP->nrows)
        {

            for(col=0 ; col < imgP->ncols; )
            {

                /* Read in the RLE count */
                GET_BYTE(ibyte, fio);
                if(ibyte & 0x80)
                {
                    /* run-length encoded pixel */
                    rle_count = ibyte - 127;
                    rle_compressed = 1;
                    /* Read the repeated byte */
                    timg.ri = &tri;
                    timg.gi = &tgi;
                    timg.bi = &tbi;
                    timg.ai = &tai;
                    if(tgaPixelRead(fio, &timg, 1, tgaP->pixel_size, tgaP->mapped))
                    {
                        fprintf(stderr,"tgaRead: read error in rle row %i\n", c_row);
                        return EOF ;
                    }

                }
                else
                {
                    /* stream of unencoded pixels */
                    rle_count = ibyte + 1;
                    rle_compressed = 0;
                }

                blir = imgP->ncols - col;

                /* Put run data into image memory */
                while(rle_count)
                {
                    if(rle_count <= blir)
                    {
                        /* finish the RLE block */
                        nbytes = rle_count;
                    }
                    else
                    {
                        /* finish the row */
                        nbytes = blir;
                    }
                    if(row_count >= imgP->nrows)
                    {
                        fprintf(stderr, "tgaRead: overread image.\n");
                        col = imgP->ncols;
                        break;
                    }
                    if(rle_compressed)
                    {
                        memset(&imgP->ri[c_row*imgP->ncols+col], timg.ri[0], nbytes);
                        memset(&imgP->gi[c_row*imgP->ncols+col], timg.gi[0], nbytes);
                        memset(&imgP->bi[c_row*imgP->ncols+col], timg.bi[0], nbytes);
                        memset(&imgP->ai[c_row*imgP->ncols+col], timg.ai[0], nbytes);
                    }
                    else
                    {
                        timg.ri = &(imgP->ri[c_row*imgP->ncols+col]);
                        timg.gi = &(imgP->gi[c_row*imgP->ncols+col]);
                        timg.bi = &(imgP->bi[c_row*imgP->ncols+col]);
                        timg.ai = &(imgP->ai[c_row*imgP->ncols+col]);
                        if(tgaPixelRead(fio, &timg,nbytes,tgaP->pixel_size, tgaP->mapped))
                        {
                            fprintf(stderr, "tgaRead: read err 3 in row %i\n", c_row);
                            return EOF ;
                        }
                    }
                    if(rle_count <= blir)
                    {
                        /* just emptied the RLE block */
                        col += rle_count;
                        rle_count = 0;
                    }
                    else
                    {
                        /* just emptied a row */
                        rle_count -= blir;
                        col = 0;
                        blir = imgP->ncols;
                        ROW_INC;
                    }
                }
            } /* for col */

            ROW_INC;
        } /* while row_count */

    }
    else
    {
        /* Not run-length encoded */
        /* load pixel data one row at a time */
        while(row_count < imgP->nrows)
        {
            timg.ri = &(imgP->ri[c_row*imgP->ncols]);
            timg.gi = &(imgP->gi[c_row*imgP->ncols]);
            timg.bi = &(imgP->bi[c_row*imgP->ncols]);
            timg.ai = &(imgP->ai[c_row*imgP->ncols]);
            if(tgaPixelRead(fio, &timg,imgP->ncols, tgaP->pixel_size, tgaP->mapped))
            {
                fprintf(stderr, "tgaRead: read error in row %i\n", c_row);
                return EOF ;
            }
            ROW_INC;
        }
    }

    return 0 ;
}

/* NAME
//   tgaHeaderRead: load a Targa image header from fio into tgaP and imgP
//
//
// DESCRIPTION
//   Since Targa files (aka tga files) have no magic number
//   it is not a simple matter to determine whether a file is a valid
//   Targa image.  Therefore, there are several consistency checks in
//   this header reading routine to try to determine whether the file
//   is a valid targa file.
//
//   In the case that the file is not a Targa file, then you could
//   lseek to the beginning of the file and try to read it as another
//   type of image.
//
//   Since there is no way to be certain of whether the error is because
//   this is not a Targa at all, or if it is because the file is simply
//   a corrupt or unsupported Targa, no error messages are reported
//   by this routine.  Instead, a different value is returned for
//   every different kind of reason why this routine rejected the
//   header.  The caller routine is responsible for handling this return
//   value appropriately.
//
//
// RETURN VALUES
//   If this routine returns nonzero, then either the file is not a
//   valid targa file, or we don't support this type.
//
*/
int tgaHeaderRead(tga_hdr_t *tgaP, RgbaImageT *imgP, FILE *fio)
{
    unsigned char flags;

    GET_BYTE(tgaP->id_len, fio);
    GET_BYTE(tgaP->cmap_type, fio);
    GET_BYTE(tgaP->img_type, fio);

    /* Verify that this is among the supported Targa types */
    switch(tgaP->img_type)
    {
    case TGA_RLE_Map:
    case TGA_RLE_RGB:
    case TGA_RLE_Mono:
        imgP->compressed = 1;
        break;

    case TGA_Map:
    case TGA_RGB:
    case TGA_Mono:
        break;

    default:
        /* This is not a Targa I can deal with */
        /* (or it is not a Targa at all) */
        return 1 ;

        /*NOTREACHED*/
        break;
    }

    imgP->type = TARGA_MAGIC;

    /* Load rest of Targa header */
    tgaP->cmap_index = get_le_word(fio);
    tgaP->cmap_len = get_le_word(fio);
    GET_BYTE(tgaP->cmap_size, fio);
    tgaP->x_off = get_le_word(fio);
    tgaP->y_off = get_le_word(fio);
    imgP->ncols = get_le_word(fio);
    imgP->nrows = get_le_word(fio);
    GET_BYTE(tgaP->pixel_size, fio);

    GET_BYTE(flags, fio);
    tgaP->att_bits = flags & 0xf;
    tgaP->reserved = (flags & 0x10) >> 4;
    tgaP->origin_bit = (flags & 0x20) >> 5;
    tgaP->interleave = (flags & 0xc0) >> 6;

    /* Load the ID field */
    if(tgaP->id_len)
    {
        char *id_field;
        id_field=MY_CALLOC(tgaP->id_len, char);

        if(get_block(fio, id_field, tgaP->id_len))
        {
            fprintf(stderr, "tgaHeaderRead: read error in id field\n");
            return EOF ;
        }
        FREE(id_field);
    }

    /* Verify the validity of the colormap or pixel size */
    if(tgaP->img_type == TGA_Map || tgaP->img_type == TGA_RLE_Map
            || tgaP->img_type == TGA_CompMap || tgaP->img_type == TGA_CompMap4 )
    {
        if(tgaP->cmap_type != 1)
        {
            /* There was no valid colormap, but one was required */
            return 2 ;
        }

        imgP->color_mapped = tgaP->mapped = 1;

        switch(tgaP->cmap_size)
        {
        case 8:
        case 24:
        case 32:
        case 15:
        case 16:
            break;

        default:
            /* invalid colormap entry size */
            return 3 ;
            /*NOTREACHED*/
            break;
        }
        imgP->pixel_size = tgaP->cmap_size;

        if(tgaP->pixel_size!=8 && tgaP->pixel_size!=15 && tgaP->pixel_size!=16)
        {
            return 7 ;
        }
    }
    else
    {

        tgaP->mapped = 0;

        switch(tgaP->pixel_size)
        {
        case 8:
        case 15:
        case 16:
        case 24:
        case 32:
            break;

        default:
            /* invalid pixel size */
            return 4 ;
            /*NOTREACHED*/
            break;
        }
        imgP->pixel_size = tgaP->pixel_size;
    }

    if(tgaP->cmap_type)
    {
        if(tgaP->cmap_index + tgaP->cmap_len > 16384)
        {
            /* colormap is invalid length */
            return 5 ;
        }

#ifdef CMAP256
        if(tgaP->cmap_index + tgaP->cmap_len > 256)
        {
            /* colormap will not fit */
            return 6 ;
        }
#endif

        tga_cmap.ri = &tga_cmap_r[tgaP->cmap_index];
        tga_cmap.gi = &tga_cmap_g[tgaP->cmap_index];
        tga_cmap.bi = &tga_cmap_b[tgaP->cmap_index];
        tga_cmap.ai = &tga_cmap_a[tgaP->cmap_index];
        tgaPixelRead(fio, &tga_cmap, tgaP->cmap_len, tgaP->cmap_size, 0);
    }
    return 0 ;
}

/* ---------------------------------------------------------------------- */

#define RPIX(row, col) (imgP->ri[(row) * imgP->ncols + (col)])
#define GPIX(row, col) (imgP->gi[(row) * imgP->ncols + (col)])
#define BPIX(row, col) (imgP->bi[(row) * imgP->ncols + (col)])
#define APIX(row, col) (imgP->ai[(row) * imgP->ncols + (col)])

/* tgaPixelWrite: save Targa pixels to fio from imgP
//
//
// ARGUMENTS
//   fio: pointer to the output image file opened for binary output.
//
//   imgP: used for the image arrays (and imgP->ncols used to index)
//
//   npixels: number of consecutive pixels to write.
//
//   mpsize: size of the pixels in memory, not the size of the
//     pixels being written.
//     -- For mpsize 8, use only the red channel.
//     -- For mpsize 15|16, use the red and green channels.
//          Use red as the MSB and green as the LSB.
//
//   mapped: tells whether pixel values are gray/coded RGB, or cmap index
//     if mpsize==8|15|16 then mapped implies that a lookup ought to be
//     done, and the mapped pixel should be written.
//
//
// [There are two kinds of map: 24/32 bit pixel and 15 bit pixel.
//   "mpsize" refers to the size of the stored image, not the written
//   image.  To date, mapped saves are not supported, so this issue is moot.]
*/
static int tgaPixelWrite(FILE *fio, RgbaImageT *imgP, int col, int row, int npixels, int mpsize, int mapped)
{
    register int pcount;

    switch(mpsize)
    {
    case 8:
        if(mapped)
        {
            fprintf(stderr, "tgaPixelWrite: I only do non-mapped 8\n");
            return EOF ;
        }
        {
            for(pcount=col; pcount < col+npixels; pcount++)
            {
                if(putc(RPIX(row, pcount), fio)==EOF) return EOF ;
            }
        }

    case 15:
    case 16:
        if(mapped)
        {
            fprintf(stderr, "tgaPixelWrite: I only do non-mapped 15/16\n");
            return EOF ;
        }
        {
            for(pcount=col; pcount < col+npixels; pcount++)
            {
                if(putc(GPIX(row, pcount), fio)==EOF) return EOF ;
                if(putc(RPIX(row, pcount), fio)==EOF) return EOF ;
            }
        }
        break;

    case 32:
    case 24:
        if(mapped)
        {
            fprintf(stderr, "tgaPixelWrite: 24/32 can't be mapped\n");
        }
        {
            for(pcount=col; pcount < col+npixels; pcount++)
            {
                if(putc(BPIX(row, pcount), fio)==EOF) return EOF ;
                if(putc(GPIX(row, pcount), fio)==EOF) return EOF ;
                if(putc(RPIX(row, pcount), fio)==EOF) return EOF ;
                if(mpsize == 32)
                {
                    if(putc(APIX(row, pcount), fio)==EOF) return EOF ;
                }
            }
        }
        break;

    default:
        fprintf(stderr, "tgaPixelWrite: bad pixel size %i\n", mpsize);
        return EOF ;
        /*NOTREACHED*/
        break;
    }
    return 0 ;
}

/* NAME
//   tgaRunLength: find RLE run length for Targa image file
//
//
// DESCRIPTION
//   tgaRunLength: find RLE run length at current col, row of img
//   depth is the number of bits per pixel
//
//   For depth 8 Use only red channel.
//   For depth 15|16 Use red as the MSB and green as the LSB.
//
//   Only runs along rows;  Will not read into next row.
//
//
// RETURN VALUES
//   If pixel repeat 2 or 3 times, return negative of number of repeats
//   otherwise Return positive number of distinct pixels until a 2|3+
//
//   If error, return 0.
//
//   A return value of 0 is indistinguishible from a start-at-end-of-row
//   occurance, so the caller must check for end-of-row before calling
//   this routine;
*/
int tgaRunLength(RgbaImageT *imgP, int col, int row, int depth)
{
    int xi, ri;
    int run_length;

    switch(depth)
    {
    case 8:
    case 15:
    case 16:
    case 24:
    case 32:
        break;

    default:
        fprintf(stderr, "tgaRunLength: invalid depth %i\n", depth);
        return 0 ;
    }

    /* Check for a run of (at least 2 or 3, at most 128) identical pixels */
    /* Don't look at the first pixel;  It's obviously equal to itself. */
    for(ri=col+1; ri<imgP->ncols && ri-col<128; ri++)
    {
        if(RPIX(row, ri) != RPIX(row, col)) break;
        if(depth>8)
        {
            if(GPIX(row, ri) != GPIX(row, col)) break;
            if(depth>16)
            {
                if(BPIX(row, ri) != BPIX(row, col)) break;
                if(depth==32)
                {
                    if(APIX(row, ri) != APIX(row, col)) break;
                }
            }
        }
    }
    run_length=ri-col;

    switch(depth)
    {
    case 8:
        if(run_length>=3) return -run_length ;
        break;

    case 15:
    case 16:
    case 24:
    case 32:
        if(run_length>=2) return -run_length ;
        break;
    }

    /* If we've reached this far, we've into a run of distinct pixels. */
    /* Look for runs of (at most 128) distinct pixels. */
    for(xi=col+1; xi<imgP->ncols && xi-col<128; xi+=run_length)
    {

        for(ri=xi+1; ri<imgP->ncols && ri-xi<3; ri++)
        {
            if(RPIX(row, ri) != RPIX(row, xi)) break;
            if(depth>8)
            {
                if(GPIX(row, ri) != GPIX(row, xi)) break;
                if(depth>16)
                {
                    if(BPIX(row, ri) != BPIX(row, xi)) break;
                    if(depth==32)
                    {
                        if(APIX(row, ri) != APIX(row, xi)) break;
                    }
                }
            }
        }
        run_length=ri-xi;

        switch(depth)
        {
        case 8:
            if(run_length>=3) return (xi-col);
            break;

        case 15:
        case 16:
        case 24:
        case 32:
            if(run_length>=2) return (xi-col);
            break;
        }
    }
    return (xi-col);
}

/* NAME
//   tgaWrite: Save a Targa image file
//
//
// DESCRIPTION
//   Save a Targa image file into fio from imgP
*/
int tgaWrite(tga_hdr_t *tgaP, RgbaImageT *imgP, FILE *fio)
{
    int   c_row;          /* current row index being saved */
    int   rl_encoded;     /* boolean flag */
    int   row_count;      /* total count of rows saved */
    int   ilace_row;      /* interlaced row counter */
    int   top_row;        /* where to start over for interlaced images */

    if(tgaP->img_type == TGA_RLE_Map || tgaP->img_type == TGA_RLE_RGB ||
            tgaP->img_type == TGA_RLE_Mono)
    {
        rl_encoded = 1;
    }
    else
    {
        rl_encoded = 0;
    }

    row_count = ilace_row = top_row = 0;
    c_row = CURR_ROW(ilace_row);
    if(rl_encoded)
    {
        int rle_count;  /* run-length of data */
        int col;        /* current column */

        while(row_count < imgP->nrows)
        {
            for(col=0; col < imgP->ncols; )
            {
                rle_count = tgaRunLength(imgP, col, c_row, tgaP->pixel_size);
                if(rle_count < 0)
                {
                    /* Write the repeat count (negative) */
                    putc(127 - rle_count, fio);

                    /* Write out the pixels */
                    if(tgaPixelWrite(fio, imgP, col, c_row, 1, tgaP->pixel_size,
                                     tgaP->mapped))
                    {
                        fprintf(stderr, "tgaWrite: write error in row %i\n", c_row);
                        return EOF ;
                    }

                    /* Advance the column counter */
                    col += -rle_count;

                }
                else if(rle_count > 0)
                {
                    /* Write the distinct count (positive) */
                    putc(rle_count - 1, fio);

                    /* Write out the pixels */
                    if(tgaPixelWrite(fio, imgP, col, c_row, rle_count,
                                     tgaP->pixel_size, tgaP->mapped))
                    {
                        fprintf(stderr, "tgaWrite: write error in row %i\n", c_row);
                        return EOF ;
                    }

                    /* Advance the column counter */
                    col += rle_count;

                }
                else
                {
                    fprintf(stderr, "tgaWrite: bad RLE count %i\n", rle_count);
                }
            }
            ROW_INC;
        }
    }
    else
    {
        /* Not run-length encoded */
        /* save pixel data one row at a time */
        while(row_count < imgP->nrows)
        {
            if(tgaPixelWrite(fio, imgP, 0, c_row, imgP->ncols, tgaP->pixel_size,
                             tgaP->mapped))
            {
                fprintf(stderr, "tgaWrite: write error in row %i\n", c_row);
                return EOF ;
            }
            ROW_INC;
        }
    }
    return 0 ;
}

/* NAME
//   tgaHeaderWrite: save a Targa image header into fio from tgaP and imgP
*/
int tgaHeaderWrite(tga_hdr_t *tgaP, RgbaImageT *imgP, FILE *fio)
{
    unsigned char flags;

    tgaP->id_len = 0;
    putc(tgaP->id_len, fio);
    putc(tgaP->cmap_type, fio);
    putc(tgaP->img_type, fio);

    /* Save rest of Targa header */
    put_le_word(tgaP->cmap_index, fio);
    put_le_word(tgaP->cmap_len, fio);
    putc(tgaP->cmap_size, fio);
    put_le_word(tgaP->x_off, fio);
    put_le_word(tgaP->y_off, fio);
    put_le_word(imgP->ncols, fio);
    put_le_word(imgP->nrows, fio);
    putc(tgaP->pixel_size, fio);

    flags  = tgaP->att_bits    & 0xf;
    flags |= (tgaP->reserved   & 0x1) << 4;
    flags |= (tgaP->origin_bit & 0x1) << 5;
    flags |= (tgaP->interleave & 0x3) << 6;
    putc(flags, fio);

    if(tgaP->cmap_type)
    {
        tgaP->mapped = 1;

        /* Save the colormap for the Targa file */
        tgaPixelWrite(fio, &tga_cmap, 0, 0, tgaP->cmap_len, tgaP->cmap_size, 0);

    }
    else
    {
        tgaP->mapped = 0;
    }

    return 0 ;
}
