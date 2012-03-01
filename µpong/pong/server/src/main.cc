#include <l4/util/util.h>
#include <l4/sys/factory.h>
#include <l4/sys/kip.h>
#include <l4/cxx/iostream>

#include "gfx-drv.h"
#include "ball.h"
#include "env.h"
#include "paddle-server.h"

#include <l4/cxx/l4iostream>
#include <l4/cxx/ipc_server>
#include <l4/cxx/main_thread>

#include <iostream>

// offscreen buffer for drawing operations
Gfx<Screen_buffer> screen;
// real frame buffer
Gfx<Screen> real_screen;

// Instanciation of the ball
Ball ball( screen.color( 0xff, 0xf0, 0x40 ), 0 );
// Instanciation of the play field
Env env(screen, real_screen, ball);

// The playfield boundaries
Line ll(10, screen.height()-2, 10, 2, false);
Line lt(2, 2, screen.width()-2, 2, false);
Line lr(screen.width()-10, 2, screen.width()-10, screen.height()-2, false);
Line lb(screen.width()-2, screen.height()-2, 2, screen.height()-2, false);

Paddle paddles[] = 
{ Paddle(0,0,&ll), Paddle(620,0,&lr), Paddle(0,0,&lt), Paddle(0,460,&lb) };

class M_thread : public cxx::Thread
{
public:
  M_thread() : Thread(true) {}
  void run() {};
};
static M_thread _main;

// Instanciation of the paddle server (own thread)
Paddle_server paddle_server(env,paddles);

extern l4_kernel_info_t __L4_KIP_ADDR__[1];

// Main server function
int main()
{
  L4::cout << "PS Hello here am I\n";

  std::cout << "KIP @ " << (void*)__L4_KIP_ADDR__ << ": " << std::hex << __L4_KIP_ADDR__->magic << '\n';

  // add the boundaries to the playfield
  env.add(&ll);
  env.add(&lt);
  env.add(&lr);
  env.add(&lb);

  // start the paddle server (thread)
  paddle_server.start(); 

  Obstacle *o;

  // main loop
  while(1)
    { 
      // draw the complete stuff (becomes visible on frame buffer)
      env.draw();

      // handle collisions
      o = env.collide();

      if (o)
        paddle_server.handle_collision(o);
#if 0
      if (o==&lpad)
	std::cout << "Left Points: " << ++lpp << '\n';
      if (o==&rpad)
	std::cout << "Right Points: " << ++rpp << '\n';
#endif
      // move the ball one step
      ball.move();
      l4_sleep(8);
    };
}

