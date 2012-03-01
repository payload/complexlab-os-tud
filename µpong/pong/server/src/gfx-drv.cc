#include "gfx-drv.h"
#include <l4/sys/kdebug.h>
#include <l4/cxx/exceptions>
#include <l4/re/video/goos>
#include <l4/re/util/cap_alloc>
#include <l4/re/env>
#include <l4/re/error_helper>

#include <typeinfo>
#include <cstring>
#include <iostream>

Screen::Screen()
{

  L4::Cap<L4Re::Video::Goos> video;
  try
    {
      video = L4Re::Env::env()->get_cap<L4Re::Video::Goos>("vesa");
      if (!video)
	throw L4::Element_not_found();
    }
  catch(L4::Base_exception const &e)
    {
      std::cout << "Error looking for vesa device: " << e.str() << '\n';
      return;
    }
  catch(...)
    {
      std::cout << "Caught unknown exception \n";
      return;
    }


  if (!video.is_valid())
    {
      std::cerr << "No video device found\n";
      return;
    }

  L4Re::Video::Goos::Info gi;

  video->info(&gi);

  L4::Cap<L4Re::Dataspace> fb = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  video->get_static_buffer(0, fb);

  if (!fb.is_valid())
    {
      std::cerr << "Invalid frame buffer object\n";
      return;
    }

  void *fb_addr = (void*)0x10000000;
  L4Re::chksys(L4Re::Env::env()->rm()->attach(&fb_addr, fb->size(), L4Re::Rm::Search_addr, fb, 0));

  if (!fb_addr)
    {
      std::cerr << "Cannot map frame buffer\n";
      return;
    }

  L4Re::Video::View v = video->view(0);
  L4Re::Video::View::Info vi;
  L4Re::chksys(v.info(&vi));

  _base        = (unsigned long)fb_addr + vi.buffer_offset;
  _line_bytes  = vi.bytes_per_line;
  _width       = vi.width;
  _height      = vi.height;
  _bpp	       = vi.pixel_info.bytes_per_pixel();

  if (   !vi.pixel_info.r().size()
      || !vi.pixel_info.g().size()
      || !vi.pixel_info.b().size())
    {
      std::cerr << "Something is wrong with the color mapping\n"
	          "assume rgb 5:6:5\n";
      _red_shift   = 11; _red_size   = 5;
      _green_shift = 5;  _green_size = 6;
      _blue_shift  = 0;  _blue_size  = 5;
    }
  else
    {
      _red_shift   = vi.pixel_info.r().shift();
      _green_shift = vi.pixel_info.g().shift();
      _blue_shift  = vi.pixel_info.b().shift();
      _red_size    = vi.pixel_info.r().size();
      _green_size  = vi.pixel_info.g().size();
      _blue_size   = vi.pixel_info.b().size();
    }

  std::cout << "\nFramebuffer: base   = 0x" << std::hex << _base
           << "\n             width  = " << std::dec << (unsigned)_width
	   << "\n             height = " << (unsigned)_height
	   << "\n             bpl    = " << _line_bytes
	   << "\n             bpp    = " << _bpp
	   << "\n             mode   = (" 
	   << _red_size << ':' << _green_size << ':' << _blue_size << ")\n";

}

