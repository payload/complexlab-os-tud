#include <stdio.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/cxx/ipc_server>

static L4Re::Util::Registry_server<> server;

class Bye_server : public L4::Server_object
{
public:
  int dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios);
};

int Bye_server::dispatch(l4_umword_t, L4::Ipc::Iostream &ios)
{
  unsigned long size;
  char *buf = NULL;
  ios >> size >> L4::Ipc::Buf_in<char>(buf, size);
  printf("%s %s\n", buf, buf);
  return 0;
}

int main()
{
  static Bye_server bye;
  if (!server.registry()->register_obj(&bye, "bye_server").is_valid())
    {
      printf("Could not register my service, readonly namespace?\n");
      return 1;
    }
  printf("Cheerio!\n");
  server.loop();
  return 0;
}
