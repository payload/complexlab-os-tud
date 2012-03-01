#include "paddle.h"

extern char _binary_paddle_img_start;

Paddle::Paddle(unsigned ox, unsigned oy, Obstacle *wall)
  : num_points(0), xo(ox), yo(oy), oxo(xo), oyo(yo),  
    ua(0,0,0,0), 
    bm(20,60,3,0,8,8,8,16,8, Bitmap::CW90, &_binary_paddle_img_start)  ,
    _wall(wall)
{}

void Paddle::add( int x, int y )
{
  if (num_points < Max_points)
    {
      points[num_points].x = x;
      points[num_points].y = y;
      num_points++;
    }
}

bool Paddle::collide( Ball &b ) const
{
  if (num_points<=1)
    return false;
  
  bool ret = false;
  
  for (unsigned i=0; i<num_points-1; ++i)
    {
      if (b.collide(points[i].x+xo,points[i].y+yo,
	    points[i+1].x+xo,points[i+1].y+yo))
	ret = true;
    }
  if (b.collide(points[num_points-1].x+xo,points[num_points-1].y+yo,
	        points[0].x+xo,points[0].y+yo))
    ret = true;

  return ret;
}

void Paddle::draw( Gfx<Screen_buffer> &gfx ) const
{
  ua.x0 = xo>oxo?oxo:xo;
  ua.y0 = yo>oyo?oyo:yo;
  ua.x1 = xo>oxo?xo+bm.xres():oxo+bm.xres();
  ua.y1 = yo>oyo?yo+bm.yres():oyo+bm.yres();
  oxo = xo;
  oyo = yo;
  bm.draw(gfx,xo,yo);
}

void Paddle::del( Gfx<Screen_buffer> &gfx ) const
{
  gfx.rect( oxo, oyo, oxo+bm.xres(), oyo+bm.yres(), 0);
}
  
Area const &Paddle::blt_area() const
{
  return ua;
}

