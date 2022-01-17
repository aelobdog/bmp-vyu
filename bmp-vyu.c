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

#include <raylib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
typedef  uint8_t byte;
typedef uint16_t u16;
typedef uint32_t u32;
typedef  int32_t i32;
typedef uint64_t u64;

// macro to cast bigger integer types to 'byte' pointers
// this is to help read variable number of bytes into
// u32 variables by treating the u32 variables like buffers
// of 'bytes' of size 4 bytes.
#define byteptr(x) ((byte *)&(x))

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

// in case the bmp file has its compression field set to '3', the
// file contains the bitmasks for extracting the rgb values from
// the data bytes.
u32 bitmasks[3] = {
   0x00ff0000,
   0x0000ff00,
   0x000000ff
};

// a global pointer of pointers that is allocated memory on the heap
// at runtime. It stores a 2D array of pixel color data - rgba.
Color **pixels;

// for bit depths 8 and below, there is a color table present, with
// 2^n entries where n is number of bits per pixel. Since value has
// to be less than 2^8 (256), here's a global array of 256 Colors to
// store the color table data.
Color color_table[256];

void print_color(Color c) {
   printf("%x %x %x %x\n", c.r, c.g, c.b, c.a);
}

// are_equal : a function that compares the first 'n' bytes of two 
// byte buffers of the same size.
byte
are_equal(byte *buffer1, byte *buffer2, int n) {
   for (int i = 0; i < n; i++) {
      if (buffer1[i] != buffer2[i]) {
         return 0;
      }
   }
   return 1;
}

// read_bytes : a function that reads 'size' number of bytes into a
// buffer of bytes, and provides an additional field to reverse the
// byte order, if that is required.
void
read_bytes(FILE *file, byte *buffer, int size, byte reverse) {
   fread(buffer, size, 1, file);
   if (reverse == 1) {
      for (int i = 0; i < size / 2; i++) {
         byte temp = buffer[i];
         buffer[i] = buffer[size - i - 1];
         buffer[size - i - 1] = temp;
      }
   }
}

// read_header : reads 16 bytes from the file into the bmp_header
// structure. Function exits if the file provided does not have a
// valid "BM" signature (in the absence of which the file is not
// a valid BMP file)
byte
read_header(bmp_header *hdr, FILE *file) {
   read_bytes(file, byteptr(hdr->type), 2, 0);
   if (!are_equal(byteptr(hdr->type), (byte *)"BM", 2)) return 1;

   read_bytes(file, byteptr(hdr->size), 4, 0);
   read_bytes(file, byteptr(hdr->reserved1), 2, 0);
   read_bytes(file, byteptr(hdr->reserved2), 2, 0);
   read_bytes(file, byteptr(hdr->offset), 4, 0);

   // @delete
   printf("bmp valid? ... yes\n");
   printf("bmp size: %d\n", hdr->size);
   printf("data offset: %d\n", hdr->offset);
   return 0;
}

// read_info_header : reads 40 bytes from the file into the 
// bmp_info_header structure.
void
read_info_header(bmp_info_header *hdr, FILE *file) {
   read_bytes(file, byteptr(hdr->info_hdr_size), 4, 0);
   read_bytes(file, byteptr(hdr->width), 4, 0);
   read_bytes(file, byteptr(hdr->height), 4, 0);
   read_bytes(file, byteptr(hdr->color_planes), 2, 0);
   read_bytes(file, byteptr(hdr->bits_pp), 2, 0);
   read_bytes(file, byteptr(hdr->compression), 4, 0);
   read_bytes(file, byteptr(hdr->image_size), 4, 0);
   read_bytes(file, byteptr(hdr->xres), 4, 0);
   read_bytes(file, byteptr(hdr->yres), 4, 0);
   read_bytes(file, byteptr(hdr->num_colors), 4, 0);
   read_bytes(file, byteptr(hdr->imp_colors), 4, 0);

   // taking absolute values because height and width appear to
   // (sometimes) get read as negative values.
   //
   // According to :
   // https://cdn.hackaday.io/files/274271173436768/Simplified%20Windows%20BMP%20Bitmap%20File%20Format%20Specification.htm#The%20Color%20Table
   //     If the image height is given as a negative 
   //     number, then the rows are ordered from top to bottom.

   hdr->width = abs(hdr->width);

   // @delete
   printf("image width : %d\n", hdr->width);
   printf("image height: %d\n", hdr->height);
   printf("compression : %d\n", hdr->compression);
   printf("bits perpix : %d\n", hdr->bits_pp);
}

void
process_color_table(bmp_info_header *hdr, FILE* file) {
   // seek till the end of the 2 headers
   fseek(file, 16 + hdr->info_hdr_size, SEEK_SET);
   printf("looking for color table @ %ld\n", ftell(file));
   printf("reading the %d entries in the color table\n", hdr->num_colors);

   // if the bit depth is 8 or less, read the colors into the color table
   if (hdr->bits_pp <= 8) {
      for (int l = 0; l < hdr->num_colors; l++) {
         u32 ct_entry = 0;
         read_bytes(file, byteptr(ct_entry), 4, 0);
         printf("# %x\n", ct_entry);
         color_table[l].a = 0xff;
         color_table[l].r = ct_entry & 0xff;
         color_table[l].g = (ct_entry >> 24) & 0xff;
         color_table[l].b = (ct_entry >> 16) & 0xff;
         printf("color : %x\n", ct_entry);
         print_color(color_table[l]);
      }
   }

   int p = 20;
   int w = 16 * p;
   int h = p;
   InitWindow(w, h, "color table");
   SetTargetFPS(60);

   while (!WindowShouldClose()) {
      BeginDrawing();
      for (int i = 0; i < 16; i++) {
         DrawRectangle(i * p, 0, p, p, color_table[i]);
      }
      EndDrawing();
   }
   CloseWindow();
}

// read_and_parse_pixel : 
void
read_and_parse_pixel(byte bits_pp, Color *c, u32 bytes) {
   c->a = 0xff; // for now, all pixels have alpha 255
   switch (bits_pp) {
      case 24: case 32:
         // if we read 4 or 3 bytes per pixel, we extract the rgba values 
         // using the masks that we read previously. 0xaabbccdd will get 
         // translated to : 
         // 0x__bbccdd & 0x00ff0000 >> 16 = 0xbb
         // 0x__bbccdd & 0x0000ff00 >>  8 = 0xcc
         // 0x__bbccdd & 0x000000ff       = 0xdd
         // and for the moment I'm assuming that the alpha value for every pixel
         // is 255 by default. So the process for 4 or 3 bytes per pixel is the
         // same.
         c->r = (bitmasks[0] & bytes) >> 16;
         c->g = (bitmasks[1] & bytes) >> 8;
         c->b = (bitmasks[2] & bytes);
         break;

      case 16:
         // using the RGB_555 format where the msbit of the 16 bit color code
         // is left out. So every color is _rrrrrgggggbbbbb
         c->r = (((bytes >> 10) & 0x1F) * 255) / 31;
         c->g = (((bytes >>  5) & 0x1F) * 255) / 31;
         c->b = (((bytes)       & 0x1F) * 255) / 31;
         break;

      case 8:
         break;

      default:
         printf("%d bit per pixel images not supported yet.\n", bits_pp);
         exit(1);
   }
}

// decompress_image : a function that fills up the pixels 2D array by reading
// the necessary data from the image file
void
decompress_image(i32 width, i32 height, bmp_header *bhdr, bmp_info_header *hdr, FILE *file) {
   byte bytes_pp = 0;

   // currently supports only reading images
   // with 8 bits per pixel or more
   bytes_pp = hdr->bits_pp / 8;

   // @delete
   printf("reading %d bytes per pixel\n", bytes_pp);

   if (hdr->bits_pp == 8) {
      process_color_table(hdr, file);
   }
   
   fseek(file, bhdr->offset, SEEK_SET);

   // the image's pixel data is stored starting from the bottom left pixel
   // going sideways and up as the rows fill up, like so
   //       5 6 7 8
   //       1 2 3 4
   // The pixel data is organized in rows from bottom to top and, within 
   // each row, from left to right.
   
   for (int i = (height < 0 ? 0 : height - 1); 
                (height < 0 ? i < -height : i >= 0);
                (height < 0 ? i++ : i--)) {

      for (int j = 0; j < width; j++) {
         if (hdr->bits_pp == 8) {
            byte c_index;
            read_bytes(file, &c_index, 1, 0);
            printf("index : %x\n", c_index);
            pixels[i][j] = color_table[c_index];
         } else {
            u32 pixel_data;
            read_bytes(file, byteptr(pixel_data), bytes_pp, 0);
            read_and_parse_pixel(hdr->bits_pp, &pixels[i][j], pixel_data);
         }
      }
   }
}

// draw_image : a function to iterate over the pixels 2D array and draw the
// desired image pixel by pixel
void
draw_image(i32 width, i32 height) {
   int p = (width > 70) ? 1 : ((height > 70) ? 1 : 10);
   int w = width * p;
   height = abs(height);
   int h = height * p;
   InitWindow(w, h, "bmp-vyu");
   SetTargetFPS(60);

   // @delete
   // for (int i = 0; i < height; i++) {
   //    for (int j = 0; j < width; j++){
   //       // printf("drawing at (%d, %d) : color {r:%x, g:%x, b:%x, a:%x}\n", i, j, pixels[i][j].r, pixels[i][j].g, pixels[i][j].b, pixels[i][j].a);
   //       //printf("%x %x %x %x  ", pixels[i][j].r, pixels[i][j].g, pixels[i][j].b, pixels[i][j].a);
   //    }
   //    printf("row : %d\n", i);
   // }

   printf("height : %d", height);
   while (!WindowShouldClose()) {
      BeginDrawing();
      for (int i = 0; i < height; i++) {
         for (int j = 0; j < width; j++) {
            DrawRectangle(j * p, i * p, p, p, pixels[i][j]);
         }
      }
      EndDrawing();
   }
   CloseWindow();
}

// main : put all the stuff in sequence
int
main(int argc, char **argv) {
   (void)argc;
   char *image;
   byte err;
   FILE *image_file;
   bmp_header hdr;
   bmp_info_header info_hdr;

   if (argc < 2) return 1;
   image = argv[1];
   image_file = fopen(image, "r");

   err = read_header(&hdr, image_file);
   if (err == 1) printf("not a valid bmp\n");

   read_info_header(&info_hdr, image_file);

   pixels = (Color **) malloc (sizeof(Color *) * abs(info_hdr.height));
   for (int i = 0; i < abs(info_hdr.height); i++) pixels[i] = (Color *)malloc(sizeof(Color) * info_hdr.width);

   decompress_image(info_hdr.width, info_hdr.height, &hdr, &info_hdr, image_file);
   draw_image(info_hdr.width, info_hdr.height);

   fclose(image_file);
   return 0;
}
