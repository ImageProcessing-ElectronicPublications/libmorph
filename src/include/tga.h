/* tga.h: Targa TrueVision image file handling routines
//
// Copyright (C) 1994-1999 by Michael J. Gourlay
//
// Provided as is.  No warrantees, express or implied.
*/
#ifndef _TGA_H__INCLUDED_
#define _TGA_H__INCLUDED_

#include <stdio.h>

#include "rgbaimage.h"

typedef struct tga_hdr_
{
    unsigned char id_len;
    unsigned char cmap_type;
    unsigned char img_type;
    int           cmap_index;
    int           cmap_len;
    unsigned char cmap_size; /* cmap entry size in bits */
    int           x_off;
    int           y_off;
    unsigned char pixel_size;
    unsigned char att_bits;
    unsigned char reserved;
    unsigned char origin_bit;   /* origin location: 0=lower 1=upper */
    unsigned char interleave;
    int           mapped;  /* whether image is colormapped (not in file) */
} tga_hdr_t;

/* Targe image types */
#define TGA_Null     0
#define TGA_Map      1
#define TGA_RGB      2
#define TGA_Mono     3
#define TGA_RLE_Map  9
#define TGA_RLE_RGB  10
#define TGA_RLE_Mono 11
#define TGA_CompMap  32
#define TGA_CompMap4 33

/* TGA_RLE is not an image type, but just a value that means "RLE" is used */
#define TGA_RLE      8

/* Interleave flag values */
#define TGA_IL_None 0
#define TGA_IL_Two  1
#define TGA_IL_Four 2

#define TARGA_MAGIC 'T' + 256 * 'G'

extern RgbaImageT tga_cmap;

extern int tgaRead(tga_hdr_t *tgaP, RgbaImageT *imgP, FILE *fio);
extern int tgaHeaderRead(tga_hdr_t *tgaP, RgbaImageT *imgP, FILE *fio);
extern int tgaWrite(tga_hdr_t *tgaP, RgbaImageT *imgP, FILE *fio);
extern int tgaHeaderWrite(tga_hdr_t *tgaP, RgbaImageT *imgP, FILE *fio);

#endif /* _TGA_H__INCLUDED_ */
