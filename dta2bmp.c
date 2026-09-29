/* Convert Comanche (1992) C?.DTA and D?.DTA maps to 8-bit BMP */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAP_W 1024
#define MAP_H 1024

typedef struct {
  int width, height;
  unsigned char *pixels;       /* width * height palette indices */
  unsigned char palette[768];  /* 256 RGB triplets, 8 bits each  */
} image_t;

static unsigned int read_u16(const unsigned char *p) {
  return p[0] | (p[1] << 8);
}

static int load_dta(const char *filename, image_t *img) {
  FILE *f;
  long size, pos, n, total;
  unsigned char *data;

  f = fopen(filename, "rb");
  if (!f) return 0;
  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);
  data = (unsigned char *)malloc(size);
  if (fread(data, 1, size, f) != (size_t)size || memcmp(data, "Kyle DTA", 8) != 0) {
    fclose(f);
    free(data);
    return 0;
  }
  fclose(f);
  img->width  = read_u16(data + 8)  + 1;   /* xmax + 1 */
  img->height = read_u16(data + 10) + 1;   /* ymax + 1 */
  total = (long)img->width * img->height;
  img->pixels = (unsigned char *)malloc(total);

  /* PCX run-length decoding, starting right after the 128-byte header */
  pos = 128;
  n = 0;
  while (n < total && pos < size) {
    unsigned char b = data[pos++];
    if ((b & 0xC0) == 0xC0) {
      int count = b & 0x3F;
      unsigned char value = data[pos++];
      while (count-- && n < total) {
        img->pixels[n++] = value;
      }
    } else {
        img->pixels[n++] = b;
    }
  }

  /* 256-color palette: marker 0x0C followed by 768 bytes at the end */
  if (size >= 769 && data[size - 769] == 0x0C) {
    memcpy(img->palette, data + size - 768, 768);
  }

  free(data);
  return 1;
}

static void write_u16(FILE *f, unsigned int v) {
  fputc(v & 0xFF, f);
  fputc((v >> 8) & 0xFF, f);
}

static void write_u32(FILE *f, unsigned long v) {
  write_u16(f, (unsigned int)(v & 0xFFFF));
  write_u16(f, (unsigned int)(v >> 16));
}

static int save_bmp(const char *filename, const image_t *img) {
  FILE *f;
  int i, y;
  unsigned long row_size = (img->width + 3) & ~3UL;    /* rows padded to 4 bytes */
  unsigned long pixel_offset = 14 + 40 + 256 * 4;
  unsigned long file_size = pixel_offset + row_size * img->height;

  f = fopen(filename, "wb");
  if (!f) {
    return 0;
  }

  /* BITMAPFILEHEADER (14 bytes) */
  fputc('B', f); fputc('M', f);
  write_u32(f, file_size);
  write_u32(f, 0);                 /* reserved */
  write_u32(f, pixel_offset);

  /* BITMAPINFOHEADER (40 bytes) */
  write_u32(f, 40);
  write_u32(f, img->width);
  write_u32(f, img->height);       /* positive = rows stored bottom-up */
  write_u16(f, 1);                 /* planes */
  write_u16(f, 8);                 /* bits per pixel */
  write_u32(f, 0);                 /* no compression */
  write_u32(f, row_size * img->height);
  write_u32(f, 2835);              /* 72 DPI */
  write_u32(f, 2835);
  write_u32(f, 256);               /* colors used */
  write_u32(f, 0);

  /* palette: BMP wants blue, green, red, reserved */
  for (i = 0; i < 256; i++) {
    fputc(img->palette[i * 3 + 2], f);
    fputc(img->palette[i * 3 + 1], f);
    fputc(img->palette[i * 3 + 0], f);
    fputc(0, f);
  }

  /* pixel rows, last row first */
  for (y = img->height - 1; y >= 0; y--) {
    unsigned long pad;
    fwrite(img->pixels + (long)y * img->width, 1, img->width, f);
    for (pad = img->width; pad < row_size; pad++) {
      fputc(0, f);
    }
  }
  fclose(f);
  return 1;
}

int main(int argc, char *argv[]) {
  char in_name[256], out_name[256];
  image_t color, height;
  int map, i;

  if (argc < 2) {
    printf("usage: dta2bmp <map number 1-4>\n");
    return 1;
  }
  map = atoi(argv[1]);

  sprintf(in_name, "./maps/C%d.DTA", map);
  if (!load_dta(in_name, &color)) {
    printf("Could not read %s\n", in_name);
    return 1;
  }
  sprintf(out_name, "map%d_color.bmp", map);
  save_bmp(out_name, &color);

  sprintf(in_name, "./maps/D%d.DTA", map);
  if (!load_dta(in_name, &height)) {
    printf("Could not read %s\n", in_name);
    return 1;
  }
  
  /* gray ramp: index = height */
  for (i = 0; i < 256; i++) {
    height.palette[i * 3 + 0] = (unsigned char)i;
    height.palette[i * 3 + 1] = (unsigned char)i;
    height.palette[i * 3 + 2] = (unsigned char)i;
  }
  sprintf(out_name, "map%d_height.bmp", map);
  save_bmp(out_name, &height);

  printf("map %d: %dx%d color + height maps saved\n", map, color.width, color.height);
  free(color.pixels);
  free(height.pixels);
  return 0;
}
