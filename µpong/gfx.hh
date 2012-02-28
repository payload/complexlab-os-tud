#pragma once

#include <l4/re/video/goos>
#include <l4/libgfxbitmap/bitmap.h>
#include <l4/libgfxbitmap/font.h>

typedef unsigned long ulong;

struct Gfx {
  L4Re::Video::View::Info _info;
  void *_addr;
  gfxbitmap_color_pix_t _fg;
  gfxbitmap_color_pix_t _bg;

  Gfx(void *_addr, L4Re::Video::View::Info _info)
    : _info(_info), _addr(_addr) {
    gfxbitmap_font_init();
    fg(0);
    bg(0xFFFFFF);
    clear();
  }

  l4re_video_view_info_t *info() {
    return (l4re_video_view_info_t*)&_info;
  }

  gfxbitmap_color_pix_t convert_color(gfxbitmap_color_t color) {
    return gfxbitmap_convert_color(info(), color);
  }

  void _fill(ulong x, ulong y, ulong w, ulong h, gfxbitmap_color_pix_t color) {
    gfxbitmap_fill((l4_uint8_t*)_addr, info(), x, y, w, h, color);
  }

  void fill(ulong x, ulong y, ulong w, ulong h) {
    _fill(x, y, w, h, _fg);
  }

  void fill() {
    fill(0, 0, _info.width, _info.height);
  }

  void clear(ulong x, ulong y, ulong w, ulong h) {
    _fill(x, y, w, h, _bg);
  }

  void clear() {
    _fill(0, 0, _info.width, _info.height, _bg);
  }

  void text(ulong x, ulong y, const char *s) {
    gfxbitmap_font_text(_addr, info(), GFXBITMAP_DEFAULT_FONT, s, GFXBITMAP_USE_STRLEN, x, y, _fg, _bg);
  }

  void fg(gfxbitmap_color_t fg) {
    _fg = convert_color(fg);
  }

  void bg(gfxbitmap_color_t bg) {
    _bg = convert_color(bg);
  }

  ulong height() {
    return _info.height;
  }

  ulong width() {
    return _info.width;
  }

  ulong font_height() {
    return gfxbitmap_font_height(GFXBITMAP_DEFAULT_FONT);
  }
};
