#include <stdio.h>
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/re/util/meta>
#include <l4/cxx/ipc_server>
#include <l4/util/bitops.h>

#include <l4/re/video/goos>
#include <l4/re/util/video/goos_fb>
#include <l4/libgfxbitmap/bitmap.h>
#include <l4/libgfxbitmap/font.h>

#include <unistd.h>

typedef unsigned long ulong;
typedef unsigned char byte;

struct Color {
  byte b;
  byte g;
  byte r;
  Color(byte r, byte g, byte b) : b(b), g(g), r(r) {}
};

L4Re::Util::Registry_server<> server;

class ByeServer : public L4::Server_object
{
  unsigned session;
public:
  ByeServer(unsigned session) : session(session) {}

  int dispatch(l4_umword_t, L4::Ipc::Iostream &ios)
  {
    unsigned long size;
    char *buf = NULL;
    ios >> L4::Ipc::Buf_in<char>(buf, size);
    printf("%s %u!!\n", buf, session);
    return 0;
  }
};

class SessionServer : public L4::Server_object
{
  unsigned session;
public:
  SessionServer() : session(0) {}

  int dispatch(l4_umword_t, L4::Ipc::Iostream &ios) {
    printf("(dispatch ");
    l4_msgtag_t tag;
    ios >> tag;
    switch (tag.label()) {
    case L4::Meta::Protocol: {
      printf("Meta (");
      int ret = L4Re::Util::handle_meta_request<L4::Factory>(ios);
      printf("))\n");
      return ret;
    }
    case L4::Factory::Protocol: {
      printf("Factory (");
      unsigned op;
      ios >> op;
      if (op != 0) return -L4_EINVAL; // magic zero 13, look at bye.cfg
      ByeServer *bye = new ByeServer(++session);
      server.registry()->register_obj(bye);
      ios << bye->obj_cap();
      printf("))\n");
      return L4_EOK;
    }
    default:
      printf("default)\n");
      return -L4_EINVAL;
    }
  }
};

int print_error(const char *s)
{
  printf("%s\n", s);
  return -1;
}

struct Gfx {
  L4Re::Util::Video::Goos_fb _goos;  
  L4Re::Video::View::Info _info;
  void *_addr;
  gfxbitmap_color_pix_t _fg;
  gfxbitmap_color_pix_t _bg;

  Gfx() : _goos("fb") {
    _goos.view_info(&_info);
    _addr = _goos.attach_buffer();
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

struct Line {
  ulong nr;
  Line *prev;
  Line *next;
  const char *text;

  Line(ulong nr, const char *text)
    : nr(nr), prev(NULL), next(NULL), text(text) {}
  Line(ulong nr, const char *text, Line *prev)
    : nr(nr), prev(prev), next(NULL), text(text) {}
  Line(ulong nr, const char *text, Line *prev, Line *next)
    : nr(nr), prev(prev), next(next), text(text) {}
};

struct TextView {
  Line *top_line;
  Line *lst_line;
  Line *cur_line;
  ulong lines;
  
  TextView()
    : top_line(NULL), lst_line(NULL), cur_line(NULL), lines(0) {}

  void append_line(const char *s) {
    if (lst_line) {
      lst_line->next = new Line(++lines, s, lst_line);
      lst_line = lst_line->next;
      cur_line = lst_line;
    }
    else {
      lst_line = new Line(++lines, s);
      top_line = lst_line;
      cur_line = lst_line;
    }
  }

  void draw(Gfx &gfx) {
    if (!lines) return;
    ulong y;
    Line *line;
    ulong line_height = gfx.font_height();
    ulong lines_per_screen = gfx.height() / line_height;

    while (top_line->nr > cur_line->nr)
      top_line = top_line->prev;
    
    line = cur_line;
    y = 1;
    while (line) {
      if (line == top_line)
	break;
      if (y == lines_per_screen)
	break;
      ++y;
      line = line->prev;
    }
    top_line = line;

    y = 0;
    while (line && y < gfx.height()) {
      if (line == cur_line) gfx.bg(0xDDDDDD);
      gfx.clear(0, y, gfx.width(), line_height);
      gfx.text(0, y, line->text);
      if (line == cur_line) gfx.bg(0xFFFFFF);
      y += line_height;
      line = line->next;
    }
  }
};

int main()
{
  printf("Let's do it!\n");

  Gfx gfx;
  TextView tv;

  for (ulong i = 0; i < 50; ++i) {
    char *s = new char[4];
    sprintf(s, "%lu", i);
    tv.append_line(s);
    tv.draw(gfx);
    usleep(50000);
  }

  while (tv.cur_line->nr > 1) {
    tv.cur_line = tv.cur_line->prev;
    tv.draw(gfx);
    usleep(50000);
  }

  SessionServer session;
  if (!server.registry()->register_obj(&session, "bye_server").is_valid())
    return print_error("Could not register my service, readonly namespace?\n");
  printf("Cheerio!\n");
  server.loop();
  return 0;
}
