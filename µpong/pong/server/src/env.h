#ifndef PONG_ENV_H__
#define PONG_ENV_H__

#include "gfx-drv.h"
#include "ball.h"

/**
 * An obstacle that the ball may collide with.
 */
class Obstacle
{
public:
  /// Is there a collision ?
  virtual bool collide( Ball &b ) const = 0;
  /// Collision means out ?
  virtual bool out() const = 0;

  /// Draw this obstacle
  virtual void draw( Gfx<Screen_buffer> &gfx ) const = 0;
  /// Delete this obstacle
  virtual void del( Gfx<Screen_buffer> &gfx ) const;
  /// which area must be copied to the real frame buffer
  virtual Area const &blt_area() const;

  virtual ~Obstacle();
};

/**
 * Just a line a obstacle. This obstacle is invisible.
 */
class Line : public Obstacle
{
public:
  Line( int x1 = -1, int y1 = -1, int x2 = -1, int y2 = -1, 
        bool out = false )
    : x1(x1), y1(y1), x2(x2), y2(y2), _out(out)
  {}

  bool out() const { return _out; }
  bool valid() const 
  { return x1!=-1 || x2!=-1 || y1!=-1 || y2!=-1; }

  void draw( Gfx<Screen_buffer> &gfx ) const;

  bool collide( Ball &b ) const;

  int x1, y1, x2, y2;
  bool _out;
};


/**
 * A point.
 */
struct Point
{
  int x, y;
};


/**
 * The play field environment. Contains the obstacles and the ball.
 * - frame buffer updates are handled by the environment.
 */
class Env
{
public:
  /**
   * Create a new environment.
   * @param buf the off-screen buffer
   * @param scr the frame buffer
   * @param ball the ball
   */
  Env( Gfx<Screen_buffer> &buf, Gfx<Screen> &scr, Ball &ball );

  /// Add an obstacle
  void add( Obstacle *o );
  /// check for collisions
  Obstacle *collide() const;
  /// draw the play field
  void draw_field() const;
  /// draw the contents (obstacles and ball)
  void draw();
  
private:
  enum {
    Max_obstacles = 20,
  };
  Obstacle *o[Max_obstacles];
  mutable unsigned last;  

  Gfx<Screen_buffer> &buf;
  Gfx<Screen> &scr;
  int xo, yo;
  Ball &ball;
};

#endif // PONG_ENV_H__i

