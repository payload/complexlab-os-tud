#ifndef PONG_GFX_H__
#define PONG_GFX_H__

class Area
{
public:
  Area( int x0, int y0, int x1, int y1 ) : x0(x0), y0(y0), x1(x1), y1(y1) {}

  int x0,y0,x1,y1;
};

class Screen_buffer
{
public:
  Screen_buffer() {}
  
  unsigned line_bytes() const  { return Line_bytes; }
  int width() const            { return Width; }
  int height() const           { return Height; }
  unsigned bpp() const         { return 2; }
  unsigned red_shift() const   { return 11; }
  unsigned red_size() const    { return 5; }
  unsigned green_shift() const { return 5; }
  unsigned green_size() const  { return 6; }
  unsigned blue_shift() const  { return 0; }
  unsigned blue_size() const   { return 5; }
  unsigned long base() const   { return (unsigned long)buffer; }

  
  unsigned long addr( unsigned x, unsigned y ) const
  { return base() + y*line_bytes() + x*bpp(); }
  
private:
  enum {
    Line_bytes = 1280,
    Height     = 480,
    Width      = 640,
  };
  mutable char buffer[ Line_bytes*Height ];
};

class Screen
{
public:
  Screen();
  
  unsigned line_bytes() const  { return _line_bytes; }
  int width() const            { return _width; }
  int height() const           { return _height; }
  unsigned bpp() const         { return _bpp; }
  unsigned red_shift() const   { return _red_shift; }
  unsigned red_size() const    { return _red_size; }
  unsigned green_shift() const { return _green_shift; }
  unsigned green_size() const  { return _green_size; }
  unsigned blue_shift() const  { return _blue_shift; }
  unsigned blue_size() const   { return _blue_size; }
  unsigned long base() const   { return _base; }

  
  unsigned long addr( unsigned x, unsigned y ) const
  { return _base + y*line_bytes() + x*bpp(); }
  
private:
  unsigned long _base;
  unsigned _line_bytes;
  int _width;
  int _height;
  unsigned _bpp;
  unsigned _red_shift, _green_shift, _blue_shift;
  unsigned _red_size, _green_size, _blue_size;
};

template< typename Scr >
class Gfx : public Scr
{
public:
  unsigned color( unsigned r, unsigned g, unsigned b ) const
  { 
    return (((r & 0xff) >> (8-this->red_size()))   << this->red_shift())
         | (((g & 0xff) >> (8-this->green_size())) << this->green_shift())
         | (((b & 0xff) >> (8-this->blue_size()))  << this->blue_shift());
  }

  void vline( unsigned x, unsigned y1, unsigned y2, unsigned c ) const;
  void hline( unsigned x1, unsigned y, unsigned x2, unsigned color ) const;
  void rect( unsigned x1, unsigned y1, unsigned x2, unsigned y2, 
             unsigned c ) const;

  template< typename OScr >
  void blind_copy( Gfx<OScr> const &gfx,
                   int x1, int y1, 
                   int x2, int y2,
		   int xd, int yd ) const;

  template< typename OScr >
  void blt( Gfx<OScr> const &gfx,
            Area const &src, int xd, int yd ) const;
  
  void fill( unsigned c ) const;
};

class Bitmap
{
public:
  enum {
    CW0, CW90, CW180, CW270
  };
  
  Bitmap(unsigned xr, unsigned yr, unsigned bpp,
         unsigned rs, unsigned _rs, 
         unsigned gs, unsigned _gs,
	 unsigned bs, unsigned _bs,
	 unsigned _o,
	 void *data)
    : _red_shift(rs), _green_shift(gs), _blue_shift(bs),
      _red_size(_rs), _green_size(_gs), _blue_size(_bs),
      _bpp(bpp), _orientation(_o), xr(xr), yr(yr), data(data)
  {}	
	 
  int xres() const { return xr; }
  int yres() const { return yr; }
  unsigned bpp() const { return _bpp; }
  unsigned red_shift() const { return _red_shift; }
  unsigned green_shift() const { return _green_shift; }
  unsigned blue_shift() const { return _blue_shift; }
  unsigned red_size() const { return _red_size; }
  unsigned green_size() const { return _blue_size; }
  unsigned blue_size() const { return _blue_size; }

  unsigned red( unsigned pix ) const 
  { return (pix >> _red_shift) & ((1<<_red_size)-1); }
  
  unsigned green( unsigned pix ) const
  { return (pix >> _green_shift) & ((1<<_green_size)-1); }
  
  unsigned blue( unsigned pix ) const
  { return (pix >> _blue_shift) & ((1<<_blue_size)-1); }

  unsigned pix( unsigned /*x*/, unsigned y ) const
  { 
    unsigned long s = (unsigned long)data;
    s += (y*xr+y)*_bpp;
    return *(unsigned*)s;
  }
  
  template< typename Scr >
  void draw( Gfx<Scr> const &gfx, int x, int y ) const;

private:
  unsigned char _red_shift, _green_shift, _blue_shift;
  unsigned char _red_size, _green_size, _blue_size;
  unsigned char _bpp;
  unsigned char _orientation;
  unsigned xr,yr;
  void *data;
};

template< typename Scr >
void Gfx<Scr>::hline( unsigned x1, unsigned y, unsigned x2, unsigned c ) const
{
  unsigned long a = this->addr(x1,y);
  unsigned long e = this->addr(x2,y);
  switch(this->bpp())
    {
    case 2:
      for (; a<=e; a+=2)
        *((unsigned short*)a) = c;
      break;
    case 4:
      for (; a<=e; a+=4)
        *((unsigned*)a) = c;
      break;
    }
      
}

template< typename Scr >
void Gfx<Scr>::rect( unsigned x1, unsigned y1,
                   unsigned x2, unsigned y2, unsigned c ) const
{
  for (unsigned y = y1; y<=y2; ++y)
    hline( x1, y, x2, c);
}

template< typename Scr >
void Gfx<Scr>::vline( unsigned x, unsigned y1, unsigned y2, 
                      unsigned c ) const
{
  unsigned long a = this->addr(x,y1);
  unsigned long e = this->addr(x,y2);
  switch(this->bpp())
    {
    case 2:
      for (; a<= e; a+=this->line_bytes()) 
	*((unsigned short*)a) = c;
      break;
    case 4:
      for (; a<= e; a+=this->line_bytes()) 
	*((unsigned*)a) = c;
      break;
    }
}

template< typename Scr >
void Gfx<Scr>::fill( unsigned c ) const
{
  unsigned long a = this->base();
  unsigned long e = this->base() + this->line_bytes()*this->height();
  switch(this->bpp())
    {
    case 2:
      for (; a<= e; a+=2) 
	*((unsigned short*)a) = c;
      break;
    case 4:
      for (; a<= e; a+=4) 
	*((unsigned*)a) = c;
      break;
    }
}

template< typename Scr >
template< typename OScr >
void Gfx<Scr>::blind_copy( Gfx<OScr> const &gfx,
                           int x1, int y1, 
                           int x2, int y2,
		           int xd, int yd ) const
{
  if (y2<0 || x2<0) return;
  if (y1>=gfx.height() || x1>=gfx.width()) return;
  if (y1<0) { yd -= y1; y1 = 0; }
  if (x1<0) { xd -= x1; x1 = 0; }
  if (yd<0) { y1 -= yd; yd = 0; }
  if (xd<0) { x1 -= xd; xd = 0; }
  if (y2>gfx.height()) { y2 = gfx.height(); }
  if (x2>gfx.width()) { x2 = gfx.width(); }
  if (yd+(y2-y1)>this->height()) { y2 = this->height() - yd + y1; }
  if (xd+(x2-x1)>this->width()) { x2 = this->width() - xd + x1; }

  for (int y = y1; y < y2; ++y,++yd)
    {
      unsigned long oa = gfx.addr(x1,y); 
      unsigned long oe = gfx.addr(x2,y);
      unsigned long a = this->addr(xd,yd);
      for (; oa < oe; oa+=gfx.bpp(), a+=this->bpp())
	*((unsigned short*)a)=*((unsigned short*)oa);
    }
}

template< typename Scr >
template< typename OScr >
void Gfx<Scr>::blt( Gfx<OScr> const &gfx,
                    Area const &src, int xo, int yo ) const
{
  if (src.x0==src.x1)
    return;
  blind_copy(gfx, src.x0, src.y0, src.x1, src.y1, src.x0+xo, src.y0+yo);
}

template< typename Scr >
void Bitmap::draw( Gfx<Scr> const &gfx, int x, int y ) const
{
  unsigned long s = (unsigned long)data;
  for (unsigned ay = 0; ay<yr; ++ay)
    {
      unsigned long d = gfx.addr(x,y+ay);
      for (unsigned ax = 0; ax<xr; ++ax)
	{
	  unsigned v = *((unsigned*)s);
	  unsigned n = gfx.color(red(v), green(v), blue(v));
	  
	  if (n)
	    *((unsigned short*)d) = n;
	  
	  s += _bpp;
	  d += gfx.bpp();
	}
    }
}
  
#endif // PONG_GFX_H__

