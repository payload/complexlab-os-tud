/**
 * \file   libc_backends/l4re_mem/mem.cc
 */
/*
 * (c) 2004-2009 Technische Universität Dresden
 * (c) 2012 Gilbert Röhrbein :P
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <l4/sys/kdebug.h>

#include <l4/re/util/cap_alloc>
#include <l4/re/dataspace>
#include <l4/re/env>
#include <l4/sys/consts.h>

/*
  [size][data]				 used chunk
  [size][fnext]				 free chunk 1
  [size][fnext,fprev]			 free chunk 2
  [size][fnext,fprev,snext]		 free chunk 3
  [size][fnext,fprev,snext,sprev,*]	 free chunk 4
  [size,prev,next][data]		 dataspace  
 */

struct Chunk {
  size_t size;
  Chunk *fnext;
  Chunk *fprev;
  Chunk *snext;
  Chunk *sprev;
  void *addr() { return &fnext; }
  Chunk *next() { return (Chunk*)((size_t)this + size); }
  static Chunk &from_addr(void *p) { return *(Chunk*)((size_t*)p - 1); }
  bool is2() { return size > sizeof(size) + 2*sizeof(Chunk*); }
  bool is3() { return size > sizeof(size) + 3*sizeof(Chunk*); }
  bool is4() { return size > sizeof(size) + 4*sizeof(Chunk*); }

  void print() {
    printf("> Chunk\n\
size  %x\n\
fnext %p\n", size, fnext);
    if (is2()) printf("fprev %p\n", fprev);
    if (is3()) printf("snext %p\n", snext);
    if (is4()) printf("sprev %p\n", sprev);
  }
};

struct Space {
  size_t size;
  Space *next;
  Space *prev;
  Chunk *chunk() { return (Chunk*)((size_t)this + sizeof(Space)); }

  void print() {
    printf("> Space\n\
size  %i\n\
next  %p\n\
prev  %p\n", size, next, prev);
  }
};

struct G {
  bool initialized;
  size_t space_size_step;
  size_t chunk_size_step;
  Chunk *free;  // free list sorted by address
  Chunk *sfree; // free list sorted by size
  Space *space;
} G = {
  false,
  L4_PAGESIZE,
  L4_PAGESIZE,
  NULL,
  NULL,
  NULL
};

size_t align_size(size_t size) { return size + size % G.chunk_size_step; }

void print_some() {
  Space *space = G.space;
  while (space) {
    space->print();
    Chunk *chunk = space->chunk();
    while ((size_t)chunk < (size_t)space + space->size) {
      chunk->print();
      chunk = chunk->next();
    }
    space = space->next;
  }
}

bool can_find(size_t ) {
  return false;
}

int allocate_dataspace(size_t &addr, size_t &size, size_t needed_size)
{
  L4::Cap<L4Re::Dataspace> ds_cap =
    L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!ds_cap.is_valid()) return -1;

  if (needed_size <= G.space_size_step)
    size = G.space_size_step;
  else
    size = G.space_size_step * (needed_size / G.space_size_step);

  long err =
    L4Re::Env::env()->mem_alloc()->alloc(size, ds_cap);
  if (err) return -2;

  err = L4Re::Env::env()->rm()
    ->attach((void**)&addr, size, L4Re::Rm::Search_addr, ds_cap, 0);
  if (err) return -3;

  return 0;
}

Chunk *malloc_init(size_t size)
{
  printf("init\n");
  size_t space_addr, space_size;
  int err;

  err = allocate_dataspace(space_addr, space_size, size);
  if (err) return NULL;

  printf("space_addr %p\n", (void*)space_addr);

  G.space = (Space*)space_addr;
  G.space->size = space_size;

  Chunk *used = G.space->chunk();
  used->size = size;
  Chunk *free = used->next();
  free->size = space_size - (space_addr + (size_t)&free);

  G.initialized = true;
  return used;
}

Chunk *malloc_space(size_t )
{
  printf("allocate and use\n");
  return 0;
}

Chunk *malloc_find(size_t )
{
  printf("find and use\n");
  return 0;
}

void *malloc(size_t size) throw ()
{
  printf(">>>>>>\n");
  if (size == 0) return NULL;
  size = align_size(size);
  Chunk *used;
  if (can_find(size))
    used = malloc_find(size);
  else if (!G.initialized)
    used = malloc_init(size);
  else
    used = malloc_space(size);

  if (used == NULL)
    printf("malloc error\n");
  else {
    printf("malloc %i at %p\n", size, used);
    print_some();
  }
  printf("<<<<<<\n");

  return used->addr();
}

void free(void *p) throw()
{
  printf("free %p\n", p);
  //Chunk &chunk = Chunk::from_addr(p);
  print_some();
}

