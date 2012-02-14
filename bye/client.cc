#include <l4/sys/err.h>
#include <l4/sys/types.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/cxx/ipc_stream>
#include <stdio.h>
#include <string.h>

int bye_call(L4::Cap<void> const &server, const char *str)
{
  L4::Ipc::Iostream s(l4_utcb());
  
  unsigned long size = strlen(str)+1;
  s << size << L4::Ipc::Buf_cp_out<const char>(str, size);
  puts(str);
  
  l4_msgtag_t res = s.call(server.cap());
  if (l4_ipc_error(res, l4_utcb()))
    return 1; // failure
  return 0; // ok
}

int main()
{
  L4::Cap<void> server = L4Re::Env::env()->get_cap<void>("bye_server");
  if (!server.is_valid())
    {
      printf("Could not get server capability!\n");
      return 1;
    }
  if (bye_call(server, "bye!!"))
    {
      printf("Error talking to server\n");
      return 1;
    }
  return 0;
}
