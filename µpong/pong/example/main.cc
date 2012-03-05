#include <l4/util/util.h>
#include <l4/re/env>
#include <l4/re/namespace>

#include <l4/cxx/exceptions>
#include <l4/cxx/ipc_stream>

#include <l4/re/util/cap_alloc>

#include <stdio.h>
#include <iostream>
#include <pthread-l4.h>

#include <l4/µpong/Hacky.hh>
#include <l4/cxx/iostream>
#include <l4/re/util/object_registry>

using namespace L4;
using namespace L4Re;
using namespace L4Re::Util;

int DEBUG;
Registry_server<> registry_server;
Object_registry *registry = registry_server.registry();

class Paddle
{
public:
  Paddle( int speed, unsigned long svr );
  void run();

  int connect();
  int lifes();
  void move( int pos );
  
  bool move_up;
  bool move_down;

private:
  unsigned long svr;
  unsigned long pad_cap;
  int speed;
};

class Main
{
public:
  void run();
};

int
Paddle::connect()
{
  L4::Ipc::Iostream s(l4_utcb());
  pad_cap = L4Re::Util::cap_alloc.alloc<void>().cap();
  while (1)
    {
      l4_sleep(1000);
      std::cout << "PC: connect to " << std::hex << svr << "\n";
      s << 1UL;
      s << L4::Small_buf(pad_cap);
      l4_msgtag_t err = s.call(svr);
      l4_umword_t fp;
      s >> fp;

      std::cout << "FP: " << fp <<  " err=" << err.raw << "\n";

      if (!l4_msgtag_has_error(err) && fp != 0)
	{
	  std::cout << "Connected to paddle " << (unsigned)fp << '\n';
	  return pad_cap;
	}
      else
	{
	  switch (l4_utcb_tcr()->error)
	    {
	    case L4_IPC_ENOT_EXISTENT:
	      std::cout << "No paddle server found, retry\n";
	      l4_sleep(100);
	      s.reset();
	      break;
	    default:
	      std::cout << "Connect to paddle failed err=0x"
		       << std::hex << l4_utcb_tcr()->error << '\n';
	      return l4_utcb_tcr()->error;
	    };
	}
    }
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

  int pos = 0;
  int c = 0;
  for (;;) {
    if (c++ % 500 == 0)
      std::cout << '(' << pthread_self() << ") Lifes: " << lifes() << '\n';
    l4_sleep(10);
    if (move_up) pos -= speed;
    if (move_down) pos += speed;
    pos = pos < 0 ? 0 : pos > 1023 ? 1023 : pos;
    move(pos);
  }
}

static l4_cap_idx_t server()
{
  L4::Cap<void> s = L4Re::Env::env()->get_cap<void>("PongServer");
  if (!s)
    throw L4::Element_not_found();

  return s.cap();
}

Paddle p0(15, server());
Paddle p1(15, server());


void *thread_fn(void* ptr)
{
    Paddle *pd = (Paddle*)ptr;
    pd->run();
    return 0;
}

struct MyHacky : Hacky {
  void key_event(bool release, l4_uint8_t, char key, bool) {
    switch (key) {
    case 'w': p0.move_up = !release; break;
    case 's': p0.move_down = !release; break;
    case 'i': p1.move_up = !release; break;
    case 'k': p1.move_down = !release; break;
    }
  }
};

void Main::run()
{
  std::cout << "Hello from pong example client\n";

  pthread_t p, q;

  pthread_create(&p, NULL, thread_fn, (void*)&p0);
  pthread_create(&q, NULL, thread_fn, (void*)&p1);

  MyHacky hacky;
  registry->register_obj(&hacky);
  hacky.connect(Env::env()->get_cap<void>("hacky"));
  registry_server.loop();
}

int main()
{
  Main().run();
  return 0;
};
