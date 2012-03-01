#include <l4/µpong/SessionServer.hh>
#include <l4/µpong/hacky.hh>
#include <l4/re/util/video/goos_fb>
#include <l4/re/util/video/goos_svr>
#include <l4/re/util/dataspace_svr>
#include <l4/cxx/iostream>
#include <vector>

using namespace L4;
using namespace L4Re;
using namespace L4Re::Util;

using Ipc::Iostream;
using L4Re::Util::Video::Goos_fb;

Registry_server<> registry_server;
Object_registry *registry = registry_server.registry();

l4_addr_t fb_addr = 0;
l4_size_t fb_size = 0;
Cap<Dataspace> fb_ds;
Goos_fb *goos_fb;
int DEBUG;

struct Vfb : Dataspace_svr, L4::Server_object {

  Cap<Dataspace> ds;
  l4_addr_t addr;

  Vfb()
    : Dataspace_svr(), L4::Server_object() {

    ds = cap_alloc.alloc<Dataspace>();
    Env::env()->mem_alloc()->alloc(fb_size, ds);
    Env::env()->rm()->attach(&addr, fb_size, Rm::Search_addr, ds, 0);
    memset((void*)addr, 0, fb_size);

    _ds_start = addr;
    _ds_size  = fb_size;
    _rw_flags = Writable;
  }

  int dispatch(l4_umword_t o, Iostream &ios) {
    o |= L4_FPAGE_X;
    return Dataspace_svr::dispatch(o, ios);
  }

  ~Vfb() throw () {}

  void ds_start(l4_addr_t addr) { _ds_start = addr; }
  l4_addr_t ds_start() { return _ds_start; }

  void unmap() {
    l4_addr_t addr = _ds_start;
    while (addr < _ds_start + _ds_size) {
      Env::env()->task()->unmap(l4_fpage(addr, 21, L4_FPAGE_RWX), L4_FP_OTHER_SPACES);
      addr += 1024 * 2048;
    }
  }
};

int cur_vfb = -1;

struct FancyServer : L4Re::Util::Video::Goos_svr, L4::Server_object {
  
  Vfb vfb;

  FancyServer() {
    registry->register_obj(&vfb);

    goos_fb->goos()->info(&_screen_info);
    goos_fb->view_info(&_view_info); // XXX init_infos() is lame
    _fb_ds = cap_cast<Dataspace>(vfb.obj_cap());

    if (cur_vfb == -1) {
      cur_vfb = 0;
      vfb.ds_start(fb_addr);
    }
  }

  int dispatch(l4_umword_t o, Iostream &ios) {
    return Goos_svr::dispatch(o, ios);
  }

  void switch_off() {
    vfb.unmap();
    vfb.ds_start(vfb.addr);
    memcpy((void*)vfb.addr, (void*)fb_addr, fb_size);
  }

  void switch_on() {
    vfb.unmap();
    vfb.ds_start(fb_addr);
    memcpy((void*)fb_addr, (void*)vfb.addr, fb_size);
  }
};

SessionServer<FancyServer> session_server(registry);

void switch_vfb(int i) {
  i %= session_server.sessions.size();
  if (i == cur_vfb) return;
  if (!session_server.sessions.empty())
    session_server.sessions[cur_vfb]->switch_off();
  session_server.sessions[i]->switch_on();
  cur_vfb = i;
  if (DEBUG) cout << "CUR VFB " << cur_vfb << "\n";
}

struct MyHacky : Hacky {
  void key_event(bool release, l4_uint8_t, char key, bool) {
    if (release) return;
    if (key == '\xe2') switch_vfb(cur_vfb - 1);
    else if (key == '\xe3') switch_vfb(cur_vfb + 1);
  }
};

int main(int argc, char **argv)
{
  DEBUG = strcmp(argv[argc-1], "DEBUG") == 0;
  printf("Let's face it!\n");
  goos_fb = new Goos_fb("fb");
  fb_addr = (l4_addr_t)goos_fb->attach_buffer();
  fb_size = goos_fb->buffer()->size();
  memset((void*)fb_addr, 0, fb_size);
  MyHacky hacky;
  registry->register_obj(&session_server, "fancy");
  registry->register_obj(&hacky);
  hacky.connect(Env::env()->get_cap<void>("hacky"));
  registry_server.loop();
  delete goos_fb;
  return 0;
}
