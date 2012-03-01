#include "env.h"
#include <l4/cxx/iostream>

Obstacle::~Obstacle() 
{}

void Obstacle::del( Gfx<Screen_buffer> & ) const 
{}

Area const &Obstacle::blt_area() const
{
  static Area a(0,0,0,0);
  return a;
}


bool Line::collide( Ball &b ) const
{
  if (valid())
    return b.collide(x1, y1, x2, y2);
  else 
    return false;
}

void Line::draw( Gfx<Screen_buffer> & /*gfx*/ ) const
{
}
  
void Env::draw_field() const
{
  buf.hline(10,0,buf.width()-10, buf.color(0xff,0,0));
  buf.hline(10,buf.height()-1,buf.width()-10, 
      buf.color(0xff,0,0));
  buf.vline(10,0,buf.height()-1, buf.color(0xff,0,0));
  buf.vline(buf.width()-10,0,buf.height()-1, 
      buf.color(0xff,0,0));
}

Env::Env( Gfx<Screen_buffer> &buf, Gfx<Screen> &scr, Ball &ball )
  : last(Max_obstacles), buf(buf), scr(scr), ball(ball)
{
  buf.fill(buf.color( 0, 0, 0x0 ));
  xo = (scr.width() - buf.width())/2;
  yo = (scr.height() - buf.height())/2;
  draw_field();
  scr.blind_copy(buf, 0, 0, buf.width(), buf.height(),xo,yo);
}


void Env::add( Obstacle *_o )
{
  for (unsigned i = 0; i<Max_obstacles; ++i)
    if (o[i]==0)
      {
        o[i] = _o;
	break;
      }
}

void Env::draw()
{
  ball.del(buf);
  for (unsigned i = 0; i<Max_obstacles; ++i)
    if (o[i]!=0)
        o[i]->del(buf);

  draw_field();

  for (unsigned i = 0; i<Max_obstacles; ++i)
    if (o[i]!=0)
        o[i]->draw(buf);

  ball.draw(buf);

  for (unsigned i = 0; i<Max_obstacles; ++i)
    if (o[i]!=0)
      scr.blt(buf, o[i]->blt_area(), xo, yo );

  scr.blind_copy(buf, 
      ball.u_x1(), ball.u_y1(),
      ball.u_x2(), ball.u_y2(),
      ball.u_x1()+xo, ball.u_y1()+yo);

#if 0
  real_screen.blind_copy(buf,
      lpad.u_x1(), lpad.u_y1(),
      lpad.u_x2(), lpad.u_y2(),
      lpad.u_x1()+xo, lpad.u_y1()+yo);
  real_screen.blind_copy(buf,
      rpad.u_x1(), rpad.u_y1(),
      rpad.u_x2(), rpad.u_y2(),
      rpad.u_x1()+xo, rpad.u_y1()+yo);
#endif
}

Obstacle *Env::collide() const
{
  Obstacle *ob = 0;
  for (unsigned i = 0; i<Max_obstacles; ++i)
    if (/*last != i &&*/ o[i] && o[i]->collide(ball))
      {
	last=i;
	//if (o[i]->out())
	  ob = o[i];
      }
  return ob;
}

