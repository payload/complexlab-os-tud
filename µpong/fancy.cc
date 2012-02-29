#include "gfx.hh"
#include "textview.hh"
#include "SessionServer.hh"
#include "hacky.hh"
#include <l4/re/util/video/goos_fb>
#include <l4/re/util/video/goos_svr>
#include <l4/re/util/dataspace_svr>
#include <l4/cxx/iostream>

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

struct Vfb : Dataspace_svr, L4::Server_object {

  Vfb()
    : Dataspace_svr(), L4::Server_object() {
    _ds_start = fb_addr;
    _ds_size  = fb_size;
    _rw_flags = Writable;

    /*
    printf("unmap!\n");
    Env::env()->task()->unmap(obj_cap().fpage(), L4_FP_OTHER_SPACES);
    printf("unmap!\n");
    */
  }

  int dispatch(l4_umword_t o, Iostream &ios) {
    cout << "Vfb dispatch!\n";
    o |= L4_FPAGE_X;
    return Dataspace_svr::dispatch(o, ios);
  }

  ~Vfb() throw () {
  }
};

struct FancyServer : L4Re::Util::Video::Goos_svr, L4::Server_object {
  
  Vfb vfb;

  FancyServer() {
    _fb_ds = goos_fb->buffer();
    goos_fb->goos()->info(&_screen_info);
    goos_fb->view_info(&_view_info); // XXX init_infos() is lame
  }

  int dispatch(l4_umword_t o, Iostream &ios) {
    cout << "Fancy dispatch!\n";
    return Goos_svr::dispatch(o, ios);
  }
};

struct MyHacky : Hacky {
  void key_event(bool release, l4_uint8_t, char key, bool) {
    if (release) return;
    if (key == '\xe2')
      cout << "left\n";
    else if (key == '\xe3')
      cout << "right\n";
  }
};

int main()
{
  printf("Let's face it!\n");

  goos_fb = new Goos_fb("fb");
  fb_addr = (l4_addr_t)goos_fb->attach_buffer();
  fb_size = goos_fb->buffer()->size();

  L4Re::Video::View::Info fb_info;
  goos_fb->view_info(&fb_info);
  Gfx gfx((void*)fb_addr, fb_info);
  gfx.fg(0xDD4444);
  gfx.fill(0, 0, 100, 100);

  cout << "Splash!!\n";

  MyHacky hacky;

  SessionServer<FancyServer> session_server(registry);
  registry->register_obj(&session_server, "fancy");
  registry->register_obj(&hacky);
  hacky.connect(Env::env()->get_cap<void>("hacky"));
  registry_server.loop();
  delete goos_fb;
  return 0;
}
