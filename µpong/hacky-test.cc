#include <stdio.h>
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/cxx/ipc_stream>

using L4::Cap;
using L4Re::Env;
using L4::Ipc::Iostream;

void err(const char *s) {
  printf("ERROR %s\n", s);
}

bool DEBUG;

int main(int argc, char **argv) {
  DEBUG = argc > 1 && strcmp(argv[1], "DEBUG") == 0;
  printf("Yeah, let's hit it!\n");

  Cap<void> keyboard = Env::env()->get_cap<void>("keyboard");
  if (!keyboard.is_valid()) {
    err("no keyboard");
    return -1;
  }

  Iostream s(l4_utcb());
  s << 1 << Env::env()->main_thread();
  s.call(keyboard.cap());
  for (;;) {
    s.reset();
    l4_umword_t src;
    // XXX want to have closed receive, but s.receive(keyboard.cap()) doesn't work
    // I have to use the capability of the irq_thread for s.receive( ) on it
    l4_msgtag_t tag = s.wait(&src);
    if (tag.has_error()) {
      printf("ERROR receive\n");
      break;
    }
    
    bool release, shift;
    l4_uint8_t scan;
    char key;
    s >> release >> scan >> key >> shift;
    if (!(key & 128) && !release)
      printf("%c\n", key == '\n' ? ' ' : key);
  }
  return 0;
}
