//   ---------------------------------------------------------------------
//   Copyright (C) 2022 Ashwin Godbole
//   ---------------------------------------------------------------------
//   This file is part of bmp-vyu.
//
//   bmp-vyu is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   bmp-vyu is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details.
//   You should have received a copy of the GNU General Public License
//   along with bmp-vyu. If not, see <https://www.gnu.org/licenses/>.
//   ---------------------------------------------------------------------

#ifndef BMP_VYU_H
#define BMP_VYU_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
typedef  uint8_t byte;
typedef uint16_t u16;
typedef uint32_t u32;
typedef  int32_t i32;
typedef uint64_t u64;

typedef struct bmp_header bmp_header;
typedef struct bmp_info_header bmp_info_header;

// at the top of the .bmp file there is always a 16 byte header.
// the header may appear to be only 14 bytes in size but the 
// compiler adds padding of 2 bytes to the 'type' field to align
// the contents of the header to 4 byte boundaries
struct bmp_header {
    u16 type;
    u32 size;
    u16 reserved1, reserved2;
    u32 offset;
};

// the bmp_header is immediately followed by the information header
// that contains essential information about the image data within
// the file like the width, height, etc. The info header is 40 bytes
// in size. As in the case of the header, the 'info_hdr_size' field
// is padded with 2 bytes by the compiler.
struct bmp_info_header {
    u16 info_hdr_size;
    i32 width, height;
    u16 color_planes, bits_pp;
    u32 compression, image_size;
    u32 xres, yres;
    u32 num_colors, imp_colors;
};

byte read_header(bmp_header *hdr, FILE *file);
void read_info_header(bmp_info_header *hdr, FILE *file);
void decompress_image(Color **pixels, i32 width, i32 height, bmp_header *bhdr, bmp_info_header *hdr, FILE *file);
void draw_image(Color **pixels, i32 width, i32 height);

#endif // BMP_VYU_H

