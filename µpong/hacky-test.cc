#include <stdio.h>
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/cxx/ipc_stream>
#include <l4/re/util/object_registry>
#include <l4/re/error_helper>

using namespace L4;
using namespace L4Re;

bool DEBUG;

struct Hacky : Server_object {
  Cap<void> &hacky;
  Hacky(Cap<void> &hacky) : hacky(hacky) {
    chkcap(hacky, "not hacky...");
  }

  l4_msgtag_t connect() {
    Ipc::Iostream ios(l4_utcb());
    ios << 1 << obj_cap();
    return ios.call(hacky.cap());
  }

  // override me!!
  virtual void key_event(bool release, l4_uint8_t scan, char key, bool shift) {
    (void)release; (void)scan; (void)key; (void)shift; // suppress warnings
  }

  int dispatch(l4_umword_t, Ipc::Iostream &ios) {
    bool release, shift;
    l4_uint8_t scan;
    char key;
    ios >> release >> scan >> key >> shift;
    if (DEBUG && !(key & 128) && !release)
      printf("%c\n", key == '\n' ? ' ' : key);
    key_event(release, scan, key, shift);
    return 0;
  }
};

int main(int argc, char **argv) {
  DEBUG = strcmp(argv[argc-1], "DEBUG") == 0;
  printf("Yeah, let's hit it!\n");
  Cap<void> hacky_cap = Env::env()->get_cap<void>("hacky");
  Hacky hacky(hacky_cap);
  Util::Registry_server<> registry_server;
  registry_server.registry()->register_obj(&hacky);
  hacky.connect();
  registry_server.loop();
  return 0;
}
