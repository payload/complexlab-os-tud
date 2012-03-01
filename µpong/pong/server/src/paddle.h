#ifndef PONG_PADDLE_H__
#define PONG_PADDLE_H__

#include "gfx-drv.h"
#include "env.h"

class Paddle : public Obstacle
{
public:
  Paddle(unsigned ox, unsigned oy, Obstacle *wall);
  
  bool collide( Ball &b ) const;
  bool out() const { return true; }

  void add( int x, int y );

  void draw( Gfx<Screen_buffer> &gfx ) const;
  void del( Gfx<Screen_buffer> &gfx ) const;
  Area const &blt_area() const;

  void move( unsigned /*x*/, unsigned y ) { /*xo = x;*/ yo = y; }
  Obstacle *wall() const { return _wall; }
  
private:
  enum {
    Max_points = 20,
  };

  unsigned num_points;
  Point points[Max_points];

  unsigned xo, yo;
  mutable unsigned oxo, oyo;
  mutable Area ua;

  Bitmap bm;
  Obstacle *_wall;
};


#endif // PONG_PADDLE_H__

