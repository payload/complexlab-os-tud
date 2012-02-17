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

size_t min(size_t a, size_t b) {
  return a<b?a:b;
}

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
    null_data();
  }

  void null_data() {
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

  void copy(Chunk *dest) {
    memcpy(dest, this, min(size, sizeof(Chunk)));
  }

  void print(const char *tag = "") {
    printf("> Chunk %p%s\n", this, tag);
    printf("size  %u\n", size);
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
size  %u\n\
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
  Chunk *free = G.free;
  Space *space = G.space;
  while (space) {
    space->print();
    Chunk *chunk = space->chunk();
    while ((size_t)chunk < (size_t)space + space->size) {
      if (free == chunk) {
	chunk->print(" FREE");
	free = free->fnext;
      }
      else chunk->print();	
      chunk = chunk->next();
    }
    space = space->next;
  }
}

bool can_find(size_t ) {
  return G.free != NULL;
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
  G.free->size = space_size - ((size_t)G.free - space_addr);

  G.initialized = true;
  return used;
}

Chunk *malloc_space(size_t )
{
  printf("allocate and use\n");
  return 0;
}

Chunk *malloc_find(size_t size)
{
  printf("find and use\n");

  Chunk *free = G.free, *used = NULL, temp;
  while (free) {
    if (free->size > size) {
      free->copy(&temp);
      used = free;
      used->size = size;
      free = used->next();
      free->size = temp.size - size;
      free->fnext = temp.fnext;
      free->fprev = temp.fprev;
      if (free->fprev) free->fprev->fnext = free;
      break;
    }
    if (free->size == size) {
      //break;
    }
    free = free->fnext;
  }
  if (free->fprev == NULL) G.free = free;

  used->null_data(); // DEBUG not necessary according to man malloc
  return used;
}

void *malloc(size_t size) throw ()
{
  printf("\n>>>>>>\n");
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




void merge_right(Chunk *free, Chunk *next) {
  Chunk *temp;
  while (*free + free->size == next) {
    free->size += next->size;
    temp = next->fnext;
    next->null();
    next = temp;
  }
  if (next) next->fprev = free;
  free->fnext = next;
}

void free(void *p) throw()
{
  if (p == NULL) {
    printf("\nfree NULL\n");
    return;
  }  
  
  Chunk
    *free = Chunk::from_addr(p),
    *next = G.free,
    *prev = NULL;

  printf("\nfree %p\n", free);
  free->null_data();
  while (next && !(next > free)) {
    prev = next;
    next = next->fnext;
  }  

  if (prev) {
    free->fnext = prev->fnext;
    merge_right(prev, free);
  }
  else {
    G.free = free;
    merge_right(free, next);
  }

  print_some();
}

