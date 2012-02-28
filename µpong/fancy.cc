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

#include <unistd.h>

#include "gfx.hh"
#include "textview.hh"
#include "SessionServer.hh"
#include <l4/re/util/dataspace_svr>

L4Re::Util::Registry_server<> registry_server;
L4Re::Util::Object_registry *registry = registry_server.registry();

using L4::Cap;
using L4Re::Dataspace;
using L4Re::Util::cap_alloc;
using L4Re::Env;
using L4Re::Util::Dataspace_svr;
using L4::Ipc::Iostream;

l4_addr_t fb_addr = 0;
l4_size_t fb_size = 0;
L4Re::Video::View::Info fb_info;

struct FancyServer : Dataspace_svr, L4::Server_object {

  FancyServer()
    : Dataspace_svr(), L4::Server_object() {
    _ds_start = fb_addr;
    _ds_size = fb_size;
    _rw_flags = Writable;
  }

  int dispatch(l4_umword_t o, Iostream &ios) {
    o |= L4_FPAGE_X;
    return Dataspace_svr::dispatch(o, ios);
  }

  ~FancyServer() throw () {
  }
};

int main()
{
  printf("Let's face it!\n");

  L4Re::Util::Video::Goos_fb fb("fb");
  L4Re::Video::View::Info _info;
  fb.view_info(&fb_info);
  fb_addr = (l4_addr_t)fb.attach_buffer();
  Cap<Dataspace> fb_ds = fb.buffer();
  fb_size = fb_ds->size();

  l4_addr_t paddr;
  l4_size_t psize;
  fb_ds->phys(0, paddr, psize);
  printf("FB_DS %p %x %p %x\n", (void*)fb_addr, fb_size, (void*)paddr, psize);
  
  SessionServer<FancyServer> session_server(registry);
  registry->register_obj(&session_server, "fancy");

  /*
  *(int*)fb_addr = 5;
  printf("%x\n", *(int*)fb_addr);

  Cap<Dataspace> ds2 = cap_alloc.alloc<L4Re::Dataspace>();
  Env::env()->mem_alloc()->alloc(ds->size(), ds2);
  printf("map %li\n",
	 ds2->map(0, Dataspace::Map_rw, (l4_addr_t)addr, (l4_addr_t)addr, (l4_addr_t)addr + ds->size()));

  printf("%x\n", *(int*)addr);

  printf("map %li\n",
	 ds->map(0, Dataspace::Map_rw, (l4_addr_t)addr, (l4_addr_t)addr, (l4_addr_t)addr + ds->size()));

  printf("%x\n", *(int*)addr);
  */

  Gfx gfx((void*)fb_addr, fb_info);
  gfx.fg(0xDD4444);
  gfx.fill(0, 0, 100, 100);
  fb.goos();

  /*
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

  tv.cur_line = tv.lst_line;
  tv.draw(gfx);
  */

  registry_server.loop();
  return 0;
}
