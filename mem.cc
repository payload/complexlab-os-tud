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
  size_t addr;
  size_t size;

  bool is_used() {
    return size & 1;
  }
  bool is_garbage() {
    return addr & 1;
  }
  Chunk &set_used() {
    if (!is_used()) size = size + 1;
    return *this;
  }
  Chunk &set_garbage() {
    if (!is_garbage()) addr = addr + 1;
    return *this;
  }
  Chunk &link() {
    return *(Chunk*)size;
  }
  Chunk *next() {
    return (Chunk*)(link().addr);
  }
  size_t _size() {
    return link().size;
  }
  void _set(size_t addr, size_t size) {
    this->addr = addr;
    link().size = size;
  }
  void print() {
    printf("C addr %x\nC size %x\n\n", addr, size);
  }
};

////

struct G {
  bool initialized;
  size_t allocate_size_step;
  size_t min_allocate_size;
  size_t biggest_free;
  unsigned biggest_free_n;
  Chunk *app_space;
  Chunk *chunk_space;
  unsigned chunks;
  Chunk *free;
} G = {
  false,
  L4_PAGESIZE,
  L4_PAGESIZE,
  0,
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
  Chunk &link = new_chunk(next, size);
  Chunk &chunk = new_chunk(addr, (size_t)&link);
  return chunk;
}

Chunk &link_chunk(Chunk &chunk, size_t next = NULL)
{
  Chunk &link = new_chunk(next, chunk.size - 1);
  chunk.size = (size_t)&link;
  return chunk;
}

Chunk &link_chunk(Chunk &chunk, Chunk *next = NULL)
{
  return link_chunk(chunk, (size_t)next);
}

void add_big_free(size_t size)
{
  if (size == G.biggest_free) {
    ++G.biggest_free_n;
  } else if (size > G.biggest_free) {
    G.biggest_free = size;
    G.biggest_free_n = 1;
  }
}

void rem_big_free(size_t size)
{
  if (size == G.biggest_free)
    if (--G.biggest_free_n == 0)
      G.biggest_free = 0;
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

  printf("app_space_addr %p\nchunk_space_addr %p\n",
	 (void*)app_space_addr, (void*)chunk_space_addr);

  Chunk *chunks = (Chunk*)chunk_space_addr;

  Chunk &chunk_space_link = chunks[G.chunks++];
  chunk_space_link.addr = NULL;
  chunk_space_link.size = chunk_space_size;

  Chunk &chunk_space = chunks[G.chunks++];
  chunk_space.addr = chunk_space_addr;
  chunk_space.size = (size_t)&chunk_space_link;
  G.chunk_space = &chunk_space;

  Chunk &app_space = new_linked_chunk(app_space_addr, app_space_size);
  Chunk &used = new_chunk(app_space.addr, size).set_used();
  Chunk &free = new_linked_chunk(app_space.addr + size, app_space._size() - size);
  
  add_big_free(free._size());

  G.free = &free;
  G.app_space = &app_space;
  G.initialized = true;

  return (void*)used.addr;
}

void count_linked_chunks(Chunk *chunk, unsigned &n, unsigned &size)
{
  n = 0;
  size = 0;
  while (chunk) {
    ++n;
    size += chunk->_size();
    chunk = chunk->next();
  }
}

void print_some()
{
  unsigned n, size;
  for (n = 0, size = 0; n < G.chunks; ++n) {
    Chunk &chunk = G.chunk_space[n];
    if (chunk.is_used())
      size += chunk.size;
  }
  printf("chunks      %i %i\n", G.chunks, size);
  printf("big free    %i %i\n", G.biggest_free_n, G.biggest_free);
  count_linked_chunks(G.free, n, size);
  printf("free        %i %i\n", n, size);
  count_linked_chunks(G.app_space, n, size);
  printf("app_space   %i %i\n", n, size);
  count_linked_chunks(G.chunk_space, n, size);
  printf("chunk_space %i %i\n", n, size);
  printf("\n");
}

void *malloc_allocate_and_use(size_t )
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

  Chunk *prev, *best = NULL;
  find_best_free_chunk(size, prev, best);
  if (best->_size() > size) {

    rem_big_free(best->_size());

    Chunk &used = new_chunk(best->addr, size).set_used();
    best->_set(best->addr + size, best->_size() - size);

    add_big_free(best->_size());

    return (void*)used.addr;
  } else { // best->_size() == size
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
  if (G.biggest_free > size)
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
  printf("free %p\n", p);
  for (unsigned i = 0; i < G.chunks; ++i) {
    Chunk &chunk = G.chunk_space[i];
    if (chunk.is_used()) {
      if ((void*)chunk.addr == p) {
	G.free = &link_chunk(chunk, G.free);
	add_big_free(chunk._size());
	break;
      }
    }
  }
  print_some();
}
