#ifndef PADDLE_THREAD_H__
#define PADDLE_THREAD_H__

#include <l4/cxx/thread>
#include "env.h"
#include "paddle.h"
#include "obj_reg.h"

#include <l4/cxx/ipc_server>
#include <l4/sys/capability>


class Paddle_so : public L4::Server_object
{
public:
  explicit Paddle_so(Paddle *_pad = 0)
  : _points(0), _py(0), _paddle(_pad), _connected(0) {}
  int dispatch(l4_umword_t obj, L4::Ipc_iostream &ios);

  void move_to(int pos);
  void set_lifes(int lifes);
  int get_lifes();

  Paddle *paddle() const { return _paddle; }

  bool connected() const { return _connected; }
  void connect(bool c = true) { _connected = c; }

  void dec_lifes() { --_points; }

private:
  int *lifes();

  int _points;
  int _py;
  Paddle *_paddle;
  bool _connected;
};


class Paddle_server : public cxx::Thread, public L4::Server_object
{
private:
  typedef L4Re::Util::Registry_server<> Server;

  Server server;

public:

  Paddle_server(Env &env, Paddle paddles[4]);

  void run();

  int dispatch(l4_umword_t obj, L4::Ipc_iostream &ios);

  L4::Cap<void> connect();

  void handle_collision( Obstacle *o );

private:

  char str[60];

  Paddle_so _pad[2];

  Env &env;
  char stack[4096];
};


#endif //PADDLE_THREAD_H__

