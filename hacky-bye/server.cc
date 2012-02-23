#include <stdio.h>
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/re/util/meta>
#include <l4/cxx/ipc_server>

#include <contrib/libio-io/l4/io/io.h>
#include <l4/util/port_io.h>

typedef unsigned long ulong;
typedef unsigned char byte;

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

using L4::Cap;
using L4::Irq;
using L4Re::Env;

#define K_ERROR "\xff"
#define K_ESC "\xff"
#define K_BACKSPACE "\xff"
#define K_LCTRL "\xff"
#define K_LSHIFT "\xfe"
#define K_RSHIFT "\xfe"
#define K_KEYPAD_MUL "\xff"
#define K_LALT "\xff"
#define K_CAPSLOCK "\xff"
#define K_F1 "\xff"
#define K_F2 "\xff"
#define K_F3 "\xff"
#define K_F4 "\xff"
#define K_F5 "\xff"
#define K_F6 "\xff"
#define K_F7 "\xff"
#define K_F8 "\xff"
#define K_F9 "\xff"
#define K_F10 "\xff"
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
#define K_F11 "\xff"
#define K_F12 "\xff"

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

int main()
{
  printf("Let's hit it!\n");

  const Env *env = Env::env();

  Cap<Irq> irq = L4Re::Util::cap_alloc.alloc<Irq>();
  if (!irq.is_valid()) return print_error("ERROR irq");

  l4io_request_irq(1, irq.cap());

  l4_msgtag_t tag = irq->attach(1, env->main_thread());
  if (tag.has_error()) return print_error("ERROR irq->attach");

  bool shift = false;
  for (;;) {
    irq->receive();
    byte s = l4util_in8(0x60);
    if (s == 0xff) continue; // error
    bool release = s & 0x80;
    if (release) s -= 0x80;
    if (s <= 0x58) {
      byte key = shift ? us_keymap_shift[s] : us_keymap[s];
      if (key & 128) {
	if (key == 0xfe)
	  shift = !release;
      } else if (!release) {
	printf("%c", key);
	fflush(stdout);
      }
    }
  }

  SessionServer session;
  if (!server.registry()->register_obj(&session, "hacky").is_valid())
    return print_error("Could not register my service, readonly namespace?\n");
  server.loop();
  return 0;
}
