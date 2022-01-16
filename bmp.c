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
typedef uint8_t byte;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;

#define byteptr(x) ((byte *)&(x))

typedef struct bmp_header bmp_header;
typedef struct bmp_info_header bmp_info_header;

struct bmp_header {
   u16 type;
   u32 size;
   u16 reserved1, reserved2;
   u32 offset;
};

struct bmp_info_header {
   u16 info_hdr_size;
   i32 width, height;
   u16 color_planes, bits_pp;
   u32 compression, image_size;
   u32 xres, yres;
   u32 num_colors, imp_colors;
};

u32 bitmasks[3];
Color **pixels;

byte
are_equal(byte *buffer1, byte *buffer2, int n) {
   for (int i = 0; i < n; i++) {
      if (buffer1[i] != buffer2[i]) {
         return 0;
      }
   }
   return 1;
}

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

byte
read_header(bmp_header *hdr, FILE *file) {
   read_bytes(file, byteptr(hdr->type), 2, 0);
   if (!are_equal(byteptr(hdr->type), (byte *)"BM", 2))
      return 1;

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

   hdr->height = abs(hdr->height);
   hdr->width = abs(hdr->width);

   // @delete
   printf("image width : %d\n", hdr->width);
   printf("image height: %d\n", hdr->height);
   printf("compression : %d\n", hdr->compression);
   printf("bits perpix : %d\n", hdr->bits_pp);
}

void
read_and_parse_pixel(byte bytes_pp, Color *c, u32 bytes) {
   c->a = 0xff; // for now, all pixels have alpha 255

   switch (bytes_pp) {
      case 4:
         c->r = (bitmasks[0] & bytes) >> 16;
         c->g = (bitmasks[1] & bytes) >> 8;
         c->b = (bitmasks[2] & bytes);
         break;
      case 3:
         printf("work in progress\n");
         exit(1);
      case 1:
         c->r = ((0xe0 & bytes) >> 5);
         c->g = ((0x1c & bytes) >> 2);
         c->b = ((0x03 & bytes));
         break;
      default:
         printf("%d bit per pixel images not supported yet.\n", bytes_pp * 8);
         exit(1);
   }
}

void
decompress_image(i32 width, i32 height, bmp_header *bhdr, bmp_info_header *hdr, FILE *file) {

   byte bytes_pp = 0;
   switch (hdr->compression) {
      case 3:
         read_bytes(file, byteptr(bitmasks), 12, 0);
         break;
   }
   
   // currently supports only reading images
   // with 8 bits per pixel or more
   bytes_pp = hdr->bits_pp / 8;

   // @delete
   printf("reading %d bytes per pixel\n", bytes_pp);

   fseek(file, bhdr->offset, SEEK_SET);
   int bytes_read = 0;
   for (int i = height - 1; i >= 0; i--) {
      for (int j = 0; j < width; j++){
         u16 temp;
         u32 temp2;
         read_bytes(file, byteptr(temp2), bytes_pp, 0);
         bytes_read += bytes_pp;
         read_and_parse_pixel(
               bytes_pp, 
               &pixels[i][j],
               temp2);

         // @delete
         // printf("reading into (%d, %d) : color {r:%x, g:%x, b:%x, a:%x}\n", i, j, pixels[i][j].r, pixels[i][j].g, pixels[i][j].b, pixels[i][j].a);
      }
   }
}

void
draw_image(i32 width, i32 height) {
   int p = 10;
   int w = width * p;
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

   pixels = (Color **) malloc (sizeof(Color *) * info_hdr.height);
   for (int i = 0; i < info_hdr.height; i++) pixels[i] = (Color *)malloc(sizeof(Color) * info_hdr.width);

   decompress_image(info_hdr.width, info_hdr.height, &hdr, &info_hdr, image_file);
   draw_image(info_hdr.width, info_hdr.height);

   fclose(image_file);
   return 0;
}
