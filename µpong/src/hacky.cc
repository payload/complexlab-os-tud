#include <l4/µpong/SessionServer.hh>
#include <stdio.h>
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <contrib/libio-io/l4/io/io.h>
#include <l4/util/port_io.h>
#include <pthread-l4.h>

using L4::Cap;
using L4::Irq;
using L4Re::Env;
using L4Re::Util::cap_alloc;


L4::Cap<void> rcv_cap;

struct Loop_hooks :
  public L4::Ipc_svr::Ignore_errors,
  public L4::Ipc_svr::Default_timeout,
  public L4::Ipc_svr::Compound_reply
{
  static void setup_wait(L4::Ipc::Istream &istr, bool) {
    rcv_cap = cap_alloc.alloc<void>();
    istr.reset();
    istr << L4::Ipc::Small_buf(rcv_cap.cap(), L4_RCV_ITEM_LOCAL_ID);
    l4_utcb_br()->bdr = 0;
  }
};

L4Re::Util::Registry_server<Loop_hooks> registry_server;
L4Re::Util::Object_registry *registry = registry_server.registry();



#define K_ERROR "\xff"
#define K_ESC "\xff"
#define K_BACKSPACE "\xff"
#define K_LCTRL "\xff"
#define K_LSHIFT "\xfe"
#define K_RSHIFT "\xfe"
#define K_KEYPAD_MUL "\xff"
#define K_LALT "\xff"
#define K_CAPSLOCK "\xff"
#define K_F1 "\xe1"
#define K_F2 "\xe2"
#define K_F3 "\xe3"
#define K_F4 "\xe4"
#define K_F5 "\xe5"
#define K_F6 "\xe6"
#define K_F7 "\xe7"
#define K_F8 "\xe8"
#define K_F9 "\xe9"
#define K_F10 "\xea"
#define K_NUMLOCK "\xff"
#define K_SCROLLLOCK "\xff"
#define K_KEYPAD_7 "\xff"
#define K_KEYPAD_8 "\xff"
#define K_KEYPAD_9 "\xff"
#define K_KEYPAD_SUB "\xff"
#define K_KEYPAD_4 "\xff"
#define K_KEYPAD_5 "\xff"
#define K_KEYPAD_6 "\xff"
#define K_KEYPAD_ADD "\xff"
#define K_KEYPAD_1 "\xff"
#define K_KEYPAD_2 "\xff"
#define K_KEYPAD_3 "\xff"
#define K_KEYPAD_0 "\xff"
#define K_KEYPAD_DOT "\xff"
#define K_ALT_SYSRQ "\xff"
#define K_UNKNOWN "\xff"
#define K_F11 "\xeb"
#define K_F12 "\xec"

bool DEBUG;

struct HackyServer : public L4::Server_object {
  l4_cap_idx_t client;
  HackyServer() : client(0) {}
  int dispatch(l4_umword_t, L4::Ipc::Iostream &ios) {
    l4_uint8_t op;
    ios >> op;
    switch (op) {
    case 1:
      client = rcv_cap.cap();
    default:
      return -L4_EINVAL;
    }
    return 0;
  }
};

SessionServer<HackyServer> session_server(registry);

const char *us_keymap =
  K_ERROR K_ESC "1234567890-=" K_BACKSPACE "\tqwertyuiop[]\n" K_LCTRL "asdfghjkl;'`" K_LSHIFT // 2a
  "\\zxcvbnm,./" K_RSHIFT K_KEYPAD_MUL K_LALT " " K_CAPSLOCK K_F1 K_F2 K_F3 K_F4 // 3e
  K_F5 K_F6 K_F7 K_F8 K_F9 K_F10 K_NUMLOCK K_SCROLLLOCK K_KEYPAD_7 K_KEYPAD_8 // 48
  K_KEYPAD_9 K_KEYPAD_SUB K_KEYPAD_4 K_KEYPAD_5 K_KEYPAD_6 K_KEYPAD_ADD // 4e
  K_KEYPAD_1 K_KEYPAD_2 K_KEYPAD_3 K_KEYPAD_0 K_KEYPAD_DOT K_ALT_SYSRQ K_UNKNOWN // 55
  K_UNKNOWN K_F11 K_F12; // 58

const char *us_keymap_shift =
  K_ERROR K_ESC "!\"§$%&/()=?" K_BACKSPACE "\tQWERTYUIOP{}\n" K_LCTRL "ASDFGHJKL;'~" K_LSHIFT // 2a
  "|ZXCVBNM,.?" K_RSHIFT K_KEYPAD_MUL K_LALT " " K_CAPSLOCK K_F1 K_F2 K_F3 K_F4 // 3e
  K_F5 K_F6 K_F7 K_F8 K_F9 K_F10 K_NUMLOCK K_SCROLLLOCK K_KEYPAD_7 K_KEYPAD_8 // 48
  K_KEYPAD_9 K_KEYPAD_SUB K_KEYPAD_4 K_KEYPAD_5 K_KEYPAD_6 K_KEYPAD_ADD // 4e
  K_KEYPAD_1 K_KEYPAD_2 K_KEYPAD_3 K_KEYPAD_0 K_KEYPAD_DOT K_ALT_SYSRQ K_UNKNOWN // 55
  K_UNKNOWN K_F11 K_F12; // 58

void err(const char *s) {
  printf("ERROR %s\n", s);
}

void broadcast_key_event(bool release, l4_uint8_t scan, char key, bool shift) {
  
  std::vector<HackyServer*>::iterator i;
  for (i  = session_server.sessions.begin();
       i != session_server.sessions.end();
       ++i) {
    if (DEBUG) printf("client %lx\n", (*i)->client);
    if ((*i)->client) {
      L4::Ipc::Iostream ios(l4_utcb());
      ios << release << scan << key << shift;
      l4_msgtag_t tag = ios.send((*i)->client);
      if (tag.has_error()) printf("ERROR %lx\n", l4_utcb_tcr()->error);
    }
  }
}

void *irq_main(void *) {
  Cap<Irq> irq = cap_alloc.alloc<Irq>();
  if (!irq.is_valid()) {
    err("no irq");
    return NULL;
  }
  l4io_request_irq(1, irq.cap());
  Cap<L4::Thread> cap(pthread_getl4cap(pthread_self()));
  l4_msgtag_t tag = irq->attach(1, cap);
  if (tag.has_error()) {
    err("attaching irq");
    return NULL;
  }

  bool shift = false;
  for (;;) {
    irq->receive();
    l4_uint8_t s = l4util_in8(0x60);
    if (s == 0xff) continue; // error
    bool release = s & 0x80;
    if (release) s -= 0x80;
    if (s <= 0x58) {
      char key = shift ? us_keymap_shift[s] : us_keymap[s];
      if (key & 128) {
	if (key == (char)0xfe)
	  shift = !release;
      } else if (!release) {
	if (DEBUG) printf("%c\n", key != '\n' ? key : ' ');
      }
      broadcast_key_event(release, s, key, shift);
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  DEBUG = argc > 1 && strcmp(argv[1], "DEBUG") == 0;
  printf("Let's hit it!\n");

  pthread_t irq_thread;
  pthread_create(&irq_thread, NULL, irq_main, NULL);

  L4::Cap<void> cap = registry->register_obj(&session_server, "hacky");
  if (!cap.is_valid()) {
    printf("ERROR registering at \"hacky\"\n");
    return -1;
  }

  registry_server.loop();

  return 0;
}
