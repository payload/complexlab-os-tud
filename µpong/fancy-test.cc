#include <stdio.h>
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/cxx/ipc_stream>
#include <l4/re/dataspace>

using L4::Cap;
using L4Re::Env;
using L4::Ipc::Iostream;
using L4Re::Dataspace;
using L4Re::Util::cap_alloc;
using L4::Ipc::Small_buf;

#include <l4/util/util.h>
#include <l4/re/error_helper>
#include "gfx.hh"

using L4Re::chksys;
using L4Re::chkcap;

bool DEBUG;

int main(int argc, char **argv) {
  DEBUG = argc > 1 && strcmp(argv[1], "DEBUG") == 0;
  printf("Yeah, let's face it!\n");

  printf("get cap\n");
  Cap<Dataspace> fancy = Env::env()->get_cap<Dataspace>("fancy");
  if (!fancy.is_valid()) {
    printf("not fancy\n");
  }
  chkcap(fancy);
  printf("fancy!\n");

  int err;
  l4_addr_t fb_addr;
  err = Env::env()->rm()->attach(&fb_addr, fancy->size(),
			   L4Re::Rm::Search_addr, fancy, 0, L4_SUPERPAGESHIFT);
  printf("attach %x %p\n", err == -L4_EINVAL, (void*)fb_addr);

  l4_addr_t paddr;
  l4_size_t psize;
  fancy->phys(0, paddr, psize);
  printf("fancy phys %p %x\n", (void*)paddr, psize);

  l4_addr_t addr = 0;
  //chksys(fancy->map(0, Dataspace::Map_rw, addr, 0, fancy->size()));

  //printf("addr %p = %x\n", (void*)addr, *(int*)addr);

  printf("FANCY %p %lx %p %x\n", (void*)fb_addr, fancy->size(), (void*)paddr, psize);

  for (int i = 0; i < 1000; i++)
    *((int*)fb_addr + i) = 0;

  l4_sleep(2000);

  for (int i = 0; i < 1000; i++)
    *((int*)fb_addr + i) = 0xFF0000;

  /*
  Iostream ios(l4_utcb());
  ios << 42;
  chksys(ios.call(fancy.cap()));
  L4Re::Video::View::Info info;
  unsigned long size;
  int x;
  ios >> x >> L4::Ipc::Buf_cp_in<L4Re::Video::View::Info>(&info, size);
  printf("x %i  info %li\n", x, info.height);
  Gfx gfx((void*)addr, info);
  gfx.fg(0x00AA00);
  gfx.fill(100, 0, 100, 100);
  */

  return 0;
}
