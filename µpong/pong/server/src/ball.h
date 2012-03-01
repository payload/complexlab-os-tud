#ifndef PONG_BALL_H__
#define PONG_BALL_H__

#include "gfx-drv.h"

// Simply the ball
class Ball
{
public:
  Ball( unsigned fg, unsigned bg ); 
      
  int x_pos() const { return x/512; }
  int y_pos() const { return y/512; }
  int spin() const { return s/512; }
  int x_speed() const { return vx; }
  int y_speed() const { return vy; }

  bool collide( int x1, int y1, int x2, int y2 );

  void draw( Gfx<Screen_buffer> const &s );
  void del( Gfx<Screen_buffer> const &s );
  void move() 
  { 
    x += x_speed(); y += y_speed(); 
#ifdef BALL_SPIN
    int const step = 50;
    if ((s<step) && (s>-step)) 
      s = 0;
    else if (s<=-step)
      s+=step;
    else
      s-=step;
#endif
  }

  int u_x1() const { return ux1; }
  int u_x2() const { return ux2; }
  int u_y1() const { return uy1; }
  int u_y2() const { return uy2; }

  void restart();
  
private:
  int distance( int x1, int y1, int x2, int y2 ) const;

  int x, y, s, vx, vy;
  int ox, oy;
  int ux1, ux2, uy1, uy2;
  int rad;

  unsigned fg, bg;

  Bitmap bm;
 
};


#endif // PONG_BALL_H__

