#include "hacky.hh"
#include "gfx.hh"
#include "textview.hh"
#include <cstring>
#include <l4/re/env>
#include <l4/re/util/object_registry>
#include <l4/cxx/iostream>
#include <l4/re/util/video/goos_fb>

using namespace L4;
using namespace L4Re;
using namespace L4Re::Util;
using L4Re::Util::Video::Goos_fb;

int DEBUG;
Registry_server<> registry_server;
Object_registry *registry = registry_server.registry();
Gfx *gfx;
TextView tv;

struct MyHacky : Hacky {
  void key_event(bool release, l4_uint8_t, char key, bool) {
    if (release) return;
    if (key & 128) return;
    if (key == '\n')
      tv.append_line(new char[1]);
    else if (tv.lines == 0) {
      tv.append_line(new char[1]);
    } else {
      tv.cur_line = tv.lst_line;
      int len = strlen(tv.cur_line->text) + 2;
      char *text = new char[len];
      strcpy(text, tv.cur_line->text);
      text[len - 2] = key;
      text[len - 1] = '\0';
      delete tv.cur_line->text;
      tv.cur_line->text = text;
    }
    tv.draw(*gfx);
  }
};

int main(int argc, char **argv)
{
  DEBUG = strcmp(argv[argc-1], "DEBUG") == 0;
  cout << "Let's spam it!\n";

  MyHacky hacky;
  registry->register_obj(&hacky);
  hacky.connect(Env::env()->get_cap<void>("hacky"));

  Goos_fb goos_fb("fancy");
  L4Re::Video::View::Info fb_info;
  goos_fb.view_info(&fb_info);
  gfx = new Gfx(goos_fb.attach_buffer(), fb_info);

  registry_server.loop();
  return 0;
}
