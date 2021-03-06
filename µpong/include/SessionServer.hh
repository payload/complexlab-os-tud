#pragma once
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/cxx/ipc_server>
#include <vector>

using L4Re::Util::Object_registry;

template <typename ServerType>
struct SessionServer : public L4::Server_object
{
  std::vector<ServerType*> sessions;
  Object_registry *registry;

  SessionServer(Object_registry *registry)
    : L4::Server_object(), registry(registry) {}
  int dispatch(l4_umword_t, L4::Ipc::Iostream &ios);
};

#include <stdio.h>
#include <l4/sys/capability>
#include <l4/re/util/meta>

#define method(type) template <typename ServerType> type SessionServer<ServerType>::

method(int) dispatch(l4_umword_t, L4::Ipc::Iostream &ios) {
  l4_msgtag_t tag;
  ios >> tag;
  switch (tag.label()) {
  case L4::Meta::Protocol: {
    int ret = L4Re::Util::handle_meta_request<L4::Factory>(ios);
    return ret;
  }
  case L4::Factory::Protocol: {
    unsigned op;
    ios >> op;
    if (op != 0) return -L4_EINVAL;
    ServerType *server = new ServerType();
    sessions.push_back(server);
    registry->register_obj(server);
    ios << server->obj_cap();
    return L4_EOK;
  }
  default:
    return -L4_EINVAL;
  }
}
