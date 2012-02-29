#pragma once
#include <l4/cxx/ipc_server>

using namespace L4;

struct Hacky : Server_object {
  l4_msgtag_t connect(Cap<void> &hacky) {
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
    key_event(release, scan, key, shift);
    return 0;
  }
};
