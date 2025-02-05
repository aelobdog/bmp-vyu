//   ---------------------------------------------------------------------
//   Copyright (C) 2022-2025 Ashwin Godbole
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

#include "bmp-vyu.h"
#include <raylib.h>

int
show_bmp(char *filename) {
    char *image;
    byte err;
    FILE *image_file;
    bmp_header hdr;
    bmp_info_header info_hdr;

    image_file = fopen(filename, "r");

    err = read_header(&hdr, image_file);
    if (err == 1) return -1;

    read_info_header(&info_hdr, image_file);

    Color **pixels = (Color **) malloc (sizeof(Color *) * abs(info_hdr.height));
    for (int i = 0; i < abs(info_hdr.height); i++) pixels[i] = (Color *)malloc(sizeof(Color) * info_hdr.width);

    decompress_image(pixels, info_hdr.width, info_hdr.height, &hdr, &info_hdr, image_file);
    draw_image(pixels, info_hdr.width, info_hdr.height);

    for (int i = 0; i < abs(info_hdr.height); i++) free(pixels[i]);
    free(pixels);
    fclose(image_file);
    return 0;
}
