#include "ball.h"
#include <l4/cxx/iostream>

#include "math.h"

extern char _binary_ball_img_start;

Ball::Ball( unsigned fg, unsigned bg ) 
  : x(50*512), y(150*512), s(0*512), vx(4*512), vy(3*512), 
    ox(50), oy(50), rad(10), fg(fg), bg(bg),
    bm( 20, 20, 3, 0, 8, 8, 8, 16, 8, Bitmap::CW0,
	&_binary_ball_img_start )
{}

void Ball::restart()
{
  x = 270*512;
  y = 120*512;
  s = 0;
  vy = -vx;
  vx = -vy;
}

int Ball::distance( int x1, int y1, int x2, int y2 ) const
{
  int vx = x_speed();
  int vy = y_speed();
  int x = (this->x + vx) / 512;
  int y = (this->y + vy) / 512;

  int m = (x2-x1);
  int n = (y2-y1);
 
  // check ball direction (only one side of the mirror is reflective)
  if ((vy*m-vx*n) > 0)
    return rad*rad+10; 

  int z = (x-x1);
  int v = (y-y1);
  int r = m*z + n*v;
  int b = m*m + n*n;
  int g = (r*512) / b;
 
  // check whether the ball hits the mirror
  if (g<0 || g>512)
    return rad*rad+10;
  
  // calculate the squre of the distance (enough to compare)
  signed long long a = n*z - m*v;
  signed long long l = a*a/b;
  return l;
}

bool Ball::collide( int x1, int y1, int x2, int y2 )
{
  // check the distance to the mirror
  if ( distance(x1,y1,x2,y2) > (rad*rad) ) 
    return false;
  
  // collosion: calculate the new velocity
  int aax = x2-x1;
  int aay = y2-y1;
#ifdef BALL_SPIN
  int ax = (aax*cos(s)-aay*sin(s))/512;
  int ay = (aay*cos(s)+aax*sin(s))/512;
#else
  int ax = aax;
  int ay = aay;
#endif
  int x = (2*(vx*ax+vy*ay))*512/(ax*ax+ay*ay);
  vx = x*ax/512 - vx;
  vy = x*ay/512 - vy;
  if (abs(vx)<20) vx = sign(vx)*20;
  if (abs(vy)<20) vy = sign(vy)*20;
  return true;
}

void Ball::draw( Gfx<Screen_buffer> const &s )
{
  int X = x_pos();
  int Y = y_pos();
  if( ox==X && oy==Y)
    return;

  unsigned xx = bm.xres()/2;
  unsigned yy = bm.yres()/2;

  ux1 = ((X<ox)?X:ox) - xx;
  ux2 = ((X<ox)?ox:X) + xx;
  uy1 = ((Y<oy)?Y:oy) - yy;
  uy2 = ((Y<oy)?oy:Y) + yy;

  bm.draw(s, X-xx, Y-yy);

  //  s.rect( X-8, Y-8, X+8, Y+8, fg );
  ox = X; oy = Y;
}

void Ball::del( Gfx<Screen_buffer> const &s )
{
  int X = x_pos();
  int Y = y_pos();
  if( ox==X && oy==Y)
    return;

  unsigned xx = (bm.xres()+1)/2;
  unsigned yy = (bm.yres()+1)/2;

  s.rect( ox-xx, oy-yy, ox+xx, oy+yy, bg );
}

