#include "hacky.hh"

#include <stdio.h>
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/object_registry>

using namespace L4;
using namespace L4Re;

bool DEBUG;

struct Test : Hacky {
  Test(Cap<void> &hacky) : Hacky(hacky) {}

  void key_event(bool release, l4_uint8_t, char key, bool) {
    if (DEBUG && !(key & 128) && !release)
      printf("%c\n", key == '\n' ? ' ' : key);
  }
};

int main(int argc, char **argv) {
  DEBUG = strcmp(argv[argc-1], "DEBUG") == 0;
  printf("Yeah, let's hit it!\n");
  Cap<void> hacky_cap = Env::env()->get_cap<void>("hacky");
  Test hacky(hacky_cap);
  Util::Registry_server<> registry_server;
  registry_server.registry()->register_obj(&hacky);
  hacky.connect();
  registry_server.loop();
  return 0;
}
