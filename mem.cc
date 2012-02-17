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
  Chunk *next() { return *this + size; }
  bool has_fprev() { return size >= 3*sizeof(size); }
  bool has_snext() { return size >= 4*sizeof(size); }
  bool has_sprev() { return size >= 5*sizeof(size); }
  void null() {
    size = 0;
    fnext = 0;
    if (has_fprev()) fprev = 0;
    if (has_snext()) snext = 0;
    if (has_sprev()) sprev = 0;
  }

  Chunk *operator + (size_t size) {
    return (Chunk*)((size_t)this + size);
  }

  Chunk *operator - (size_t size) {
    return (Chunk*)((size_t)this - size);
  }

  static Chunk *from_addr(void*p) {
    return *(Chunk*)p - sizeof(size_t);
  }

  void print() {
    printf("> Chunk %p\n", this);
    printf("size  %x\n", size);
    printf("fnext %p\n", fnext);
    if (has_fprev()) printf("fprev %p\n", fprev);
    if (has_snext()) printf("snext %p\n", snext);
    if (has_sprev()) printf("sprev %p\n", sprev);
  }
};

////////////////////////////////////////////////////////////

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
  3*sizeof(size_t),
  NULL,
  NULL,
  NULL
};

size_t align_size(size_t size) {
  if (size < G.chunk_size_step)
    return G.chunk_size_step;
  else
    return size % G.chunk_size_step;
}

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
  G.sfree = G.free = used->next();
  G.free->size = space_size - (space_addr + (size_t)G.free);

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
  if (p == NULL) {
    printf("free NULL\n");
    return;
  }
  
  Chunk
    *free = Chunk::from_addr(p),
    *next = G.free,
    *prev = NULL,
    temp;

  printf("free %p %p\n", free, p);

  while (next && !(next > free)) {
    prev = next;
    next = next->fnext;
  }
  
  // merge right
  while (*free + free->size == next) {
    printf("merge right\n");
    free->size += next->size;
    temp = *next;
    next->null();
    next = temp.fnext;
  }
  if (next) next->fprev = free;
  free->fnext = next;

  // merge left
  while (prev && *prev + prev->size == free) {
    printf("merge left\n");
    prev->size += free->size;
    prev->fnext = next;
    next->fprev = prev;
    free = prev;
    prev = prev->fprev;
    prev->fnext = free;
  }
  free->fprev = prev;

  print_some();
}

