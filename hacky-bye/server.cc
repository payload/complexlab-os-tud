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

int main()
{
  printf("Let's hit it!\n");

  const Env *env = Env::env();

  Cap<Irq> irq = L4Re::Util::cap_alloc.alloc<Irq>();
  if (!irq.is_valid()) return print_error("ERROR irq");

  l4io_request_irq(1, irq.cap());

  l4_msgtag_t tag = irq->attach(1, env->main_thread());
  if (tag.has_error()) return print_error("ERROR irq->attach");

  for (;;) {
    irq->receive();
    byte s = l4util_in8(0x60);
    printf("%x\n", s);
  }

  SessionServer session;
  if (!server.registry()->register_obj(&session, "hacky").is_valid())
    return print_error("Could not register my service, readonly namespace?\n");
  server.loop();
  return 0;
}
