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

void print_goos_info(L4Re::Video::View::Info &info)
{
  printf("==========\n");
  printf("Goos info %p\n", &info);
  printf(" width    %lu\n", info.width);
  printf(" height   %lu\n", info.height);
  printf(" flags    %u\n", info.flags);
  //printf(" views    %u\n", info.num_static_views);
  //printf(" buffers  %u\n", info.num_static_buffers);
  printf(" bpp      %u\n", info.pixel_info.bits_per_pixel());
  printf(" rsize    %u\n", info.pixel_info.r().size());
  printf(" gsize    %u\n", info.pixel_info.g().size());
  printf(" bsize    %u\n", info.pixel_info.b().size());
  printf(" asize    %u\n", info.pixel_info.a().size());
  printf(" rshift   %u\n", info.pixel_info.r().shift());
  printf(" gshift   %u\n", info.pixel_info.g().shift());
  printf(" bshift   %u\n", info.pixel_info.b().shift());
  printf(" ashift   %u\n", info.pixel_info.a().shift());
  printf("==========\n");
}

typedef unsigned long ulong;
typedef unsigned char byte;

struct Color {
  byte b;
  byte g;
  byte r;
  Color(byte r, byte g, byte b) : b(b), g(g), r(r) {}
};

struct FB : public L4Re::Video::View::Info {
  void *addr;
  L4Re::Util::Video::Goos_fb goos;
  ulong linesize;
  ulong size;

  FB() : goos("fb") {
    goos.view_info(this);
    linesize = l4util_next_power2(width * bpp());
    size = linesize * height;
  }

  L4Re::Video::View::Info *info() const {
    return (L4Re::Video::View::Info*)this;
  }

  unsigned char bpp() const {
    return pixel_info.bytes_per_pixel();
  }

  Color *pxl(ulong x, ulong y) {
    return (Color*)((ulong)addr + x*bpp() + y*linesize);
  }

  void draw_pixel(ulong x, ulong y, Color c) {
    *(Color*)pxl(x,y) = c;
  }

  void refresh() {
    goos.refresh(0, 0, width, height);
  }
};

int main()
{
  printf("Let's do it!\n");
  FB fb;
  fb.addr = fb.goos.attach_buffer();
  print_goos_info(fb);
  memset(fb.addr, 0xFF, fb.size);

  SessionServer session;
  if (!server.registry()->register_obj(&session, "bye_server").is_valid())
    return print_error("Could not register my service, readonly namespace?\n");
  printf("Cheerio!\n");
  server.loop();
  return 0;
}
