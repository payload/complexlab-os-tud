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
    {  std::cout << "PC: connect to " << std::hex << svr << "\n";
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
	      l4_sleep(1000);
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
  L4::Cap<void> s = L4Re::Env::env()->get_cap<void>("PongServer");
  if (!s)
    throw L4::Element_not_found();

  return s.cap();
}

Paddle p0(-10, server());
Paddle p1(20, server());


void *thread_fn(void* ptr)
{
    Paddle *pd = (Paddle*)ptr;
    pd->run();
    return 0;
}


void Main::run()
{
  std::cout << "Hello from pong example client\n";

  pthread_t p, q;

  pthread_create(&p, NULL, thread_fn, (void*)&p0);
  pthread_create(&q, NULL, thread_fn, (void*)&p1);

  std::cout << "PC: main sleep......\n";
  l4_sleep_forever();

}

int main()
{
  Main().run();
  return 0;
};
