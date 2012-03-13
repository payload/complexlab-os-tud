#include <stdio.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/re/util/meta>
#include <l4/cxx/ipc_server>

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

int main()
{
  SessionServer session;
  if (!server.registry()->register_obj(&session, "bye_server").is_valid()) {
    printf("Could not register my service, readonly namespace?\n");
    return 1;
  }
  printf("Cheerio!\n");
  server.loop();
  return 0;
}
