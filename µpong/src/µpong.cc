/*
#include <l4/util/util.h>
#include <l4/re/env>
#include <l4/re/namespace>

#include <l4/cxx/exceptions>
#include <l4/cxx/ipc_stream>

#include <l4/re/util/cap_alloc>

#include <stdio.h>
#include <iostream>
#include <pthread-l4.h>

class Paddle
{
public:
  Paddle( int speed, unsigned long svr );
  void run();

  int connect();
  int lifes();
  void move( int pos );

private:
  unsigned long svr;
  unsigned long pad_cap;
  int speed;
};

int
Paddle::connect()
{

  return 0;
}

int Paddle::lifes()
{
  L4::Ipc::Iostream s(l4_utcb());
  s << 3UL;
  if (!l4_msgtag_has_error((s.call(pad_cap))))
    {
      int l;
      s >> l;
      return l;
    }

  return -1;
}

void Paddle::move( int pos )
{
  L4::Ipc::Iostream s(l4_utcb());
  s << 1UL << pos;
  s.call(pad_cap);
  l4_sleep(10);
}

Paddle::Paddle(int speed, unsigned long svr)
  : svr(svr), speed(speed)
{}

void Paddle::run()
{
  std::cout << "Pong client running...\n";
  int paddle = connect();
  if (paddle == -1)
    return;

  int pos = 180;

  int c = 0;
  while(1)
    {
      if (c++ >= 500)
	{
	  c = 0;
	  std::cout << '(' << pthread_self() << ") Lifes: " << lifes() << '\n';
	}
      move(pos);
      pos += speed;
      if (pos<0)
	{
	  pos = 0;
	  speed = -speed;
	}
      if (pos>1023)
	{
	  pos = 1023;
	  speed = -speed;
	}
    }
}

static l4_cap_idx_t server()
{

  if (!s)
    throw L4::Element_not_found();

  return s.cap();
}

Paddle p0(-10, server());
Paddle p1(20, server());
*/

#include <l4/µpong/Hacky.hh>
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/object_registry>
#include <l4/cxx/iostream>
#include <l4/re/error_helper>
#include <l4/util/util.h>

using namespace L4;
using namespace L4Re;
using namespace L4Re::Util;

int DEBUG;
Registry_server<> registry_server;
Object_registry *registry = registry_server.registry();

l4_cap_idx_t connect(Cap<void> server) {
  Ipc::Iostream s(l4_utcb());
  for (;;) {
    cout << "CONNECTING\n";
    l4_cap_idx_t pad_cap = cap_alloc.alloc<void>().cap();
    s << 1UL;
    s << Small_buf(pad_cap);
    cout << "CALL\n";
    l4_msgtag_t err = s.call(server.cap());
    cout << "CALL DONE\n";
    l4_umword_t fp;
    s >> fp;
    
    cout << "FP: " << fp <<  " err=" << err.raw << "\n";
    chksys(err);
    
    if (!l4_msgtag_has_error(err) && fp != 0) {
      cout << "CONNECTED " << (unsigned)fp << "\n";
      return pad_cap;
    } else {
      switch (l4_utcb_tcr()->error) {
      case L4_IPC_ENOT_EXISTENT:
	cout << "NOT FOUND, RETRY\n";
	l4_sleep(1000);
	s.reset();
	break;
      default:
	cout << "ERROR\n";
	return l4_utcb_tcr()->error;
      };
    }
  }
}

struct MyHacky : Hacky {
  void key_event(bool release, l4_uint8_t, char key, bool) {
    
  }
};

Cap<void> paddle;

int main(int argc, char **argv) {
  try {
    cout << "Yeah, let's play tennis!\n";
    
    Cap<void> pong = Env::env()->get_cap<void>("pong");
    chkcap(pong);

    Cap<void> paddle(connect(pong));
    chkcap(paddle);
    
    MyHacky hacky;
    registry->register_obj(&hacky);
    registry_server.loop();
    return 0;
  } catch(Runtime_error &e) {
    cerr << e << "\n";
    cout << "woot!";
  }
  return -1;
};
