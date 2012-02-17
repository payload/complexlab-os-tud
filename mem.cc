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

struct Chunk {
  size_t size;
  Chunk *fnext;
  Chunk *fprev;

  void *addr() { return &fnext; }
  Chunk *next() { return *this + size; }
  
  void null() {
    size = 0;
    null_data();
  }

  void null_data() {
    fnext = 0;
    fprev = 0;
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
    // when I implement the three special chunks of size 8, 16, and 20 ;)
    // [size,fnext] [size,fnext,fprev,snext] [size,fnext,fprev,snext,sprev]
  }

  void print(const char *tag = "") {
    printf("C this  %p%s\n", this, tag);
    printf("C size  %u\n", size);
  }

  void print_free() {
    print(" FREE");
    printf("C fnext %p\n", fnext);
    printf("C fprev %p\n", fprev);
  }
};

////////////////////////////////////////////////////////////

struct Space {
  size_t size;
  Space *next;
  Space *prev;
  Chunk *chunk() { return (Chunk*)((size_t)this + sizeof(Space)); }

  void print() {
    printf("S this  %p\n", this);
    printf("S size  %u\n", size);
    printf("S next  %p\n", next);
    printf("S prev  %p\n", prev);
  }
};

struct G {
  bool initialized;
  size_t space_size_step;
  size_t chunk_size_step;
  Chunk *free;  // free list sorted by address
  Chunk *sfree; // free list sorted by size - not used now
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
    return size + size % G.chunk_size_step;
}

void print_some() {
  printf("G.free  %p\n", G.free);
  Chunk *free = G.free;
  Space *space = G.space;
  while (space) {
    space->print();
    Chunk *chunk = space->chunk();
    while ((size_t)chunk < (size_t)space + space->size) {
      if (free == chunk) {
	chunk->print_free();
	free = free->fnext;
      }
      else chunk->print();	
      chunk = chunk->next();
    }
    space = space->next;
  }
}

void find_place_in_free(Chunk *free, Chunk *&prev, Chunk *&next)
{
  while (next && !(next > free)) {
    prev = next;
    next = next->fnext;
  }  
}

bool can_find(size_t ) {
  return G.free != NULL;
}

int allocate_dataspace(size_t &addr, size_t &size, size_t needed_size)
{
  needed_size += sizeof(Space) + G.chunk_size_step;

  L4::Cap<L4Re::Dataspace> ds_cap =
    L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!ds_cap.is_valid()) return -1;

  if (needed_size <= G.space_size_step)
    size = G.space_size_step;
  else
    size = G.space_size_step * (needed_size / G.space_size_step + 1);

  long err =
    L4Re::Env::env()->mem_alloc()->alloc(size, ds_cap);
  if (err) return -2;

  err = L4Re::Env::env()->rm()
    ->attach((void**)&addr, size, L4Re::Rm::Search_addr, ds_cap, 0);
  if (err) return -3;

  return 0;
}

void release_space(Space *space)
{
  int err;
  L4::Cap<L4Re::Dataspace> ds_cap;
  err = L4Re::Env::env()->rm()->detach(space, &ds_cap);
  if (err) return;
  err = L4Re::Env::env()->mem_alloc()->free(ds_cap);
  if (err) return;
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

Chunk *malloc_space(size_t size)
{
  printf("allocate and use\n");
  size_t saddr, ssize;
  int err;
  err = allocate_dataspace(saddr, ssize, size);
  if (err) return NULL;

  Space *space = (Space*)saddr, *prev = NULL, *next = G.space;
  space->size = ssize;
  while (next && !(next > space)) {
    prev = next;
    next = next->next;
  }  
  space->next = next;
  space->prev = prev;
  if (prev) prev->next = space;
  else G.space = space;
  if (next) next->prev = space;
  
  Chunk *used = space->chunk();
  used->size = size;

  size_t free_size = ssize - size - sizeof(Space);
  if (free_size >= G.chunk_size_step) {
    Chunk *free = used->next();
    free->size = free_size;
    free->fnext = G.free;
    find_place_in_free(free, free->fprev, free->fnext);
    if (free->fprev == NULL) G.free = free;
    else free->fprev->fnext = free;
    if (free->fnext) free->fnext->fprev = free;
  }

  return used;
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
      else G.free = free;
      break;
    }
    if (free->size == size) {
      if (free->fprev) free->fprev->fnext = free->fnext;
      if (free->fnext) {
	free->fnext->fprev = free->fprev;
	if (free->fprev == NULL)
	  G.free = free->fnext;
      }
      used = free;
      break;
    }
    free = free->fnext;
  }

  if (used) used->null_data(); // DEBUG not necessary according to man malloc
  return used;
}

void *malloc(size_t size) throw ()
{
  printf("\n>>>>>>\n");
  if (size == 0) return NULL;
  size = align_size(size);
  Chunk *used = NULL;
  if (can_find(size))
    used = malloc_find(size);
  else if (!G.initialized)
    used = malloc_init(size);
  if (used == NULL)
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

void collect_space(Chunk *free)
{
  Space *want = (Space*)(*free - sizeof(Space));
  Space *space = G.space;
  while (space) {
    if (space == want
	&& space->size == free->size + sizeof(Space)) {
      if (space->next) space->next->prev = space->prev;
      if (space->prev) space->prev->next = space->next;
      if (space->prev == NULL && space->next) G.space = space->next;
      if (space->prev == NULL && space->next == NULL) {
	printf("\nempty!!!\n");
      } else
	release_space(space);
      break;
    }
    space = space->next;
  }
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
  find_place_in_free(free, prev, next);
  if (prev) {
    free->fnext = prev->fnext;
    merge_right(prev, free);
  } else G.free = free;
  merge_right(free, next);

  if ((free->size + sizeof(Space)) % G.space_size_step == 0)
    collect_space(free);

  print_some();
}

