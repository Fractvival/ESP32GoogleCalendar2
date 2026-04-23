#ifndef ARDUINO

#include <ctype.h>
#include <ft2build.h>
#include <stdint.h>
#include <stdio.h>
#include FT_GLYPH_H
#include FT_MODULE_H
#include FT_TRUETYPE_DRIVER_H
#include "../gfxfont.h"

#define DPI 141

void enbit(uint8_t value) {
  static uint8_t row = 0, sum = 0, bit = 0x80, firstCall = 1;
  if (value) sum |= bit;
  if (!(bit >>= 1)) {
    if (!firstCall) {
      if (++row >= 12) {
        printf(",\n  ");
        row = 0;
      } else {
        printf(", ");
      }
    }
    printf("0x%02X", sum);
    sum = 0;
    bit = 0x80;
    firstCall = 0;
  }
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    fprintf(stderr, "Usage: %s fontfile size\n", argv[0]);
    return 1;
  }

  int size = atoi(argv[2]);

  // 🔥 CHARSET (ASCII + CZ)
  uint16_t charset[] = {
    // ASCII
    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
    48,49,50,51,52,53,54,55,56,57,
    58,59,60,61,62,63,64,
    65,66,67,68,69,70,71,72,73,74,75,76,77,
    78,79,80,81,82,83,84,85,86,87,88,89,90,
    91,92,93,94,95,96,
    97,98,99,100,101,102,103,104,105,106,107,108,109,
    110,111,112,113,114,115,116,117,118,119,120,121,122,
    123,124,125,126,

    // CZ malé
    0x00E1,0x010D,0x010F,0x00E9,0x011B,0x00ED,
    0x0148,0x00F3,0x0159,0x0161,0x0165,0x00FA,
    0x016F,0x00FD,0x017E,

    // CZ velké
    0x00C1,0x010C,0x010E,0x00C9,0x011A,0x00CD,
    0x0147,0x00D3,0x0158,0x0160,0x0164,0x00DA,
    0x016E,0x00DD,0x017D
  };

  int charset_size = sizeof(charset)/sizeof(charset[0]);

  char *fontName;
  FT_Library library;
  FT_Face face;
  FT_Glyph glyph;
  FT_Bitmap *bitmap;
  FT_BitmapGlyphRec *g;
  GFXglyph *table;

  fontName = malloc(64);
  table = malloc(charset_size * sizeof(GFXglyph));

  sprintf(fontName, "CZFont%dpt", size);

  FT_Init_FreeType(&library);

  FT_UInt interpreter_version = TT_INTERPRETER_VERSION_35;
  FT_Property_Set(library, "truetype", "interpreter-version",
                  &interpreter_version);

  FT_New_Face(library, argv[1], 0, &face);
  FT_Set_Char_Size(face, size << 6, 0, DPI, 0);

  printf("const uint8_t %sBitmaps[] PROGMEM = {\n  ", fontName);

  int bitmapOffset = 0;

  for (int i = 0; i < charset_size; i++) {

    uint16_t unicode = charset[i];

    if (FT_Load_Char(face, unicode, FT_LOAD_TARGET_MONO)) continue;
    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO)) continue;
    if (FT_Get_Glyph(face->glyph, &glyph)) continue;

    bitmap = &face->glyph->bitmap;
    g = (FT_BitmapGlyphRec *)glyph;

    table[i].bitmapOffset = bitmapOffset;
    table[i].width = bitmap->width;
    table[i].height = bitmap->rows;
    table[i].xAdvance = face->glyph->advance.x >> 6;
    table[i].xOffset = g->left;
    table[i].yOffset = 1 - g->top;

    for (int y = 0; y < bitmap->rows; y++) {
      for (int x = 0; x < bitmap->width; x++) {
        int byte = x / 8;
        uint8_t bit = 0x80 >> (x & 7);
        enbit(bitmap->buffer[y * bitmap->pitch + byte] & bit);
      }
    }

    int n = (bitmap->width * bitmap->rows) & 7;
    if (n) {
      n = 8 - n;
      while (n--) enbit(0);
    }

    bitmapOffset += (bitmap->width * bitmap->rows + 7) / 8;

    FT_Done_Glyph(glyph);
  }

  printf(" };\n\n");

  printf("const GFXglyph %sGlyphs[] PROGMEM = {\n", fontName);

  for (int i = 0; i < charset_size; i++) {
    printf("  { %5d, %3d, %3d, %3d, %4d, %4d }",
      table[i].bitmapOffset,
      table[i].width,
      table[i].height,
      table[i].xAdvance,
      table[i].xOffset,
      table[i].yOffset);

    if (i < charset_size - 1)
      printf(",   // U+%04X\n", charset[i]);
  }

  printf(" };\n\n");

  printf("const GFXfont %s PROGMEM = {\n", fontName);
  printf("  (uint8_t  *)%sBitmaps,\n", fontName);
  printf("  (GFXglyph *)%sGlyphs,\n", fontName);
  printf("  0x00, 0x%02X, %ld };\n\n", charset_size-1,
         face->size->metrics.height >> 6);

  FT_Done_FreeType(library);

  return 0;
}

#endif