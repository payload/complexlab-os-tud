#include <cstdio>
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/cxx/ipc_stream>
#include <l4/re/dataspace>
#include <l4/re/video/goos>
#include <l4/re/util/video/goos_fb>
#include <l4/util/util.h>
#include <l4/re/error_helper>
#include <l4/cxx/exceptions>
#include <l4/cxx/iostream>
#include <l4/cxx/l4iostream>
#include "gfx.hh"
#include <sstream>

using L4::Cap;
using L4Re::Env;
using L4::Ipc::Iostream;
using L4Re::Dataspace;
using L4Re::Util::cap_alloc;
using L4::Ipc::Small_buf;
using L4Re::Video::Goos;
using L4Re::chksys;
using L4Re::chkcap;
using L4Re::Util::Video::Goos_fb;

using L4::Runtime_error;

using namespace L4;

bool DEBUG;

int main(int argc, char **argv) {
  DEBUG = strcmp(argv[argc-1], "DEBUG") == 0;
  unsigned color;
  std::stringstream(argv[1]) >> std::hex >> color;
  color |= 0xFF000000;
  cout << "Yeah, let's face it!\n" << hex << color << "\n";
  try {
    l4_addr_t fb_addr;
    l4_size_t fb_size;
    Goos_fb fb("fancy");
    L4Re::Video::View::Info fb_info;
    fb.view_info(&fb_info);
    fb_addr = (l4_addr_t)fb.attach_buffer();
    Cap<Dataspace> fb_ds = fb.buffer();
    fb_size = fb_ds->size();

    cout << "Fancy!\n";

    Gfx gfx((void*)fb_addr, fb_info);
    gfx.fg(color);
    for (;;)
      gfx.fill(100, 100, 200, 200);

  } catch(Runtime_error &e) {
    cerr << e;
  }
  return 0;
}
