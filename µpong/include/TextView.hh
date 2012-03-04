#pragma once

#include "Gfx.hh"
#include <string>
#include <l4/cxx/iostream>

typedef unsigned long ulong;
using namespace std;

struct Line {
  ulong nr;
  Line *prev;
  Line *next;
  const char *text;

  Line(ulong nr, const char *text)
    : nr(nr), prev(NULL), next(NULL), text(text) {}
  Line(ulong nr, const char *text, Line *prev)
    : nr(nr), prev(prev), next(NULL), text(text) {}
  Line(ulong nr, const char *text, Line *prev, Line *next)
    : nr(nr), prev(prev), next(next), text(text) {}
};

extern int MALLOC_DEBUG;

#include <string.h>

struct TextView {
  Line *top_line;
  Line *lst_line;
  Line *cur_line;
  ulong lines;
  
  TextView()
    : top_line(NULL), lst_line(NULL), cur_line(NULL), lines(0) {}

  void append_line(const char *s) {
    if (lst_line) {
      lst_line->next = new Line(++lines, s, lst_line);
      lst_line = lst_line->next;
      cur_line = lst_line;
    }
    else {
      lst_line = new Line(++lines, s);
      top_line = lst_line;
      cur_line = lst_line;
    }
  }

  void append_text(const char *chars) {
    unsigned i;
    unsigned p = 0;
    string s(chars);
    while ((i = s.find("\n", p)) != s.npos) {
      if (i-p == 0)
	append_line(new char(0));
      else {
	char *line = new char[i-p+1];
	strncpy(line, (char*)((unsigned)chars)+p, i-p);
	line[i-p] = 0;
	append_line(line);
      }
      p = i+1;
    }
  }

  void draw(Gfx &gfx) {
    if (!lines) return;

    gfx.clear();

    ulong y;
    Line *line;
    ulong line_height = gfx.font_height();
    ulong lines_per_screen = gfx.height() / line_height;

    while (top_line->nr > cur_line->nr)
      top_line = top_line->prev;
    
    line = cur_line;
    y = 1;
    while (line) {
      if (line == top_line)
	break;
      if (y == lines_per_screen)
	break;
      ++y;
      line = line->prev;
    }
    top_line = line;

    y = 0;
    while (line && y < gfx.height()) {
      if (line == cur_line) gfx.bg(0xDDDDDD);
      gfx.clear(0, y, gfx.width(), line_height);
      gfx.text(0, y, line->text);
      if (line == cur_line) gfx.bg(0xFFFFFF);
      y += line_height;
      line = line->next;
    }
  }
};
