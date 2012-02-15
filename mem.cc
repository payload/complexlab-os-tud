/**
 * \file   libc_backends/l4re_mem/mem.cc
 */
/*
 * (c) 2004-2009 Technische Universität Dresden
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
  estimated malloc usage
  1. way more used than free chunks
  2. sequential memory usage
  2.1. don't interleave app data with malloc data
  2.2. multiple mallocs should use sequential free chunks
  2.3. also sequential free chunks?
  3. common malloc size allows alignment for 2 or 4 bytes

  1. -> used chunks should be small, free chunks could be bigger
  2.1. -> different dataspaces for app and malloc data
  2.2. -> malloc sessions, based on size, time/counter, free() calls
  2.3. -> optimize for sequential free chunks? (linked list of arrays)
  3. -> 1 or 2 unused bits in Chunk.addr and Chunk.size!!
 */

////

struct Chunk {
  size_t size;
  size_t addr;

  bool is_used() {
    return size & 1;
  }
  bool is_garbage() {
    return addr & 1;
  }
  Chunk &set_used() {
    size = size | 1;
    return *this;
  }
  void set_garbage() {
    addr = addr | 1;
  }
  Chunk *link() {
    return (Chunk*)addr;
  }
  Chunk *next() {
    return (Chunk*)(link()->size);
  }
  size_t link_addr() {
    return link()->addr;
  }
  void print() {
    printf("addr %x\nsize %x\n", addr, size);
  }
};

////

struct G {
  bool initialized;
  size_t allocate_size_step;
  size_t min_allocate_size;
  size_t biggest_free_chunk_size;
  Chunk *app_space;
  Chunk *chunk_space;
  unsigned chunks;
  Chunk *free;
} G = {
  false,
  L4_PAGESIZE,
  L4_PAGESIZE,
  0,
  NULL,
  NULL,
  0,
  NULL
};

/////////////////////

Chunk &new_chunk(size_t addr = 0, size_t size = 0)
{
  Chunk *chunks = (Chunk*)G.chunk_space->addr;
  Chunk &chunk = chunks[G.chunks++];
  chunk.addr = addr;
  chunk.size = size;
  return chunk;
}

Chunk &new_linked_chunk(size_t addr, size_t size, size_t next = NULL)
{ 
  Chunk &link = new_chunk(addr, next);
  Chunk &chunk = new_chunk((size_t)&link, size);
  return chunk;
}


//////////////

int allocate_dataspace(size_t &addr, size_t &size, size_t needed_size)
{
  L4::Cap<L4Re::Dataspace> ds_cap =
    L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!ds_cap.is_valid()) return -1;

  if (needed_size <= G.min_allocate_size)
    size = G.min_allocate_size;
  else
    size = G.allocate_size_step * (needed_size / G.allocate_size_step);

  long err =
    L4Re::Env::env()->mem_alloc()->alloc(size, ds_cap);
  if (err) return -2;

  err = L4Re::Env::env()->rm()
    ->attach((void**)&addr, size, L4Re::Rm::Search_addr, ds_cap, 0);
  if (err) return -3;

  return 0;
}

void *malloc_init(size_t size)
{
  printf("init\n");
  size_t app_space_addr, app_space_size, chunk_space_addr, chunk_space_size;
  int err;

  err = allocate_dataspace(app_space_addr, app_space_size, size);
  if (err) return 0;

  err = allocate_dataspace(chunk_space_addr, chunk_space_size, G.min_allocate_size);
  if (err) return 0;

  printf("app_space_addr %p\nchunk_space_addr %p\n", app_space_addr, chunk_space_addr);

  Chunk *chunks = (Chunk*)chunk_space_addr;

  Chunk &chunk_space_link = chunks[G.chunks++];
  chunk_space_link.addr = chunk_space_addr;
  chunk_space_link.size = NULL;

  Chunk &chunk_space = chunks[G.chunks++];
  chunk_space.addr = (size_t)&chunk_space_link;
  chunk_space.size = chunk_space_size;
  G.chunk_space = &chunk_space;

  Chunk &app_space = new_linked_chunk(app_space_addr, app_space_size);
  Chunk &used = new_chunk(app_space.link_addr(), size).set_used();
  Chunk &free = new_linked_chunk(app_space.link_addr() + size, app_space.size - size);
  
  G.biggest_free_chunk_size = free.size;
  G.free = &free;
  G.app_space = &app_space;
  G.initialized = true;

  return (void*)used.addr;
}

unsigned count_linked_chunks(Chunk *chunk)
{
  unsigned n = 0;
  while (chunk) {
    ++n;
    chunk = chunk->next();
  }
  return n;
}

void print_some()
{
  //  printf("free        %p\n", G.free);
  printf("free        %i\n", count_linked_chunks(G.free));
  //  printf("app_space   %p\n", G.app_space);
  printf("app_space   %i\n", count_linked_chunks(G.app_space));
  printf("chunk_space %i\n", count_linked_chunks(G.chunk_space));
}

void *malloc_allocate_and_use(size_t size)
{
  printf("allocate and use\n");
  /*Space *ds = allocate_dataspace(size);
  if (!ds) return 0;
  Chunk &chunk = create_free_chunk(ds, size);
  use_chunk(chunk);
  return chunk.addr;
  */
  return 0;
}

void find_best_free_chunk(size_t size, Chunk *&prev_best, Chunk *&best)
{
  Chunk *prev = NULL;
  Chunk *free = G.free;
  //Chunk *prev_best = NULL;
  //Chunk *best = NULL;
  size_t best_diff = -1;
  while (free) {
    if (free->size > size) {
      size_t diff = free->size - size;
      if (diff <= best_diff) {
	best = free;
	best_diff = diff;
	prev_best = prev;
	if (best_diff == 0) break;
      }
    }
    prev = free;
    free = free->next();
  }
}

void *malloc_find_and_use(size_t size)
{
  printf("find and use\n");

  Chunk *prev, *best;
  find_best_free_chunk(size, prev, best);
  if (best->size > size) {
    Chunk &used = new_chunk(best->link_addr(), size);
    best->addr = best->link_addr() + size;
    best->size -= size;
    return (void*)used.addr;
  }
  return 0;
}

void *malloc(size_t size) throw ()
{
  printf(">>>>>>\n");

  // something like alignment, safes a bit in Chunk.addr for used/free distinction
  // it is also more common two allocate a structure of n*2 bytes
  if (size % 2 == 1) ++size;

  void *addr;
  if (G.biggest_free_chunk_size > size)
    addr = malloc_find_and_use(size);
  else if (!G.initialized)
    addr = malloc_init(size);
  else
    addr = malloc_allocate_and_use(size);

  printf("malloc %i at %p\n", size, addr);
  print_some();
  printf("<<<<<<\n");

  return addr;
}

void free(void *p) throw()
{
  enter_kdebug("free");
}
