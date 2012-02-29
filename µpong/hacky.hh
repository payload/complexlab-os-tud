#include <l4/cxx/ipc_server>
#include <l4/re/error_helper>

using namespace L4;
using namespace L4Re;

struct Hacky : Server_object {
  Cap<void> &hacky;
  Hacky(Cap<void> &hacky) : hacky(hacky) {
    chkcap(hacky, "not hacky...");
  }

  l4_msgtag_t connect() {
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
