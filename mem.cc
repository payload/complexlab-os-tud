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

struct Dataspace {
  void *addr;
  size_t size;
  size_t free;
  Dataspace *next;
};

////

struct Chunk;

struct LinkedChunk {
  void *addr;
  Chunk *next;
};

struct Chunk {
  size_t size;
  void *addr;

  bool is_free() { return size & 1; }
  void free() { ++size; }
  void use() { --size; }
  Chunk *free_next() { return ((LinkedChunk*)addr)->next; }
  void  *free_addr() { return ((LinkedChunk*)addr)->addr; }
};

////

struct G {
  bool initialized;
  size_t allocate_size_step;
  size_t min_allocate_size;
  size_t biggest_free_chunk_size;
  Dataspace *app_space;
  Dataspace *chunk_space;
  Dataspace *link_space;;
  Chunk *free;
} G = {
  false,
  L4_PAGESIZE,
  L4_PAGESIZE,
  0,
  NULL,
  NULL,
  NULL,
  NULL
};

void		*malloc_allocate_and_use(size_t size);
void		*malloc_find_and_use(size_t size);
Dataspace	*allocate_dataspace(size_t size);
Chunk		&create_free_chunk(Dataspace *ds, size_t size);
void		 use_chunk(Chunk &chunk);
Chunk		&find_best_free_chunk(size_t size);
void		 split_chunk(Chunk &chunk, size_t size);

int allocate_dataspace(Dataspace &ds, size_t size)
{
  L4::Cap<L4Re::Dataspace> ds_cap =
    L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!ds_cap.is_valid()) return -1;

  if (size <= G.min_allocate_size)
    ds.size = G.min_allocate_size;
  else
    ds.size = G.allocate_size_step * ((size / G.allocate_size_step) + 1);

  long err =
    L4Re::Env::env()->mem_alloc()->alloc(ds.size, ds_cap);
  if (err) return -2;

  err = L4Re::Env::env()->rm()
    ->attach(&ds.addr, ds.size, L4Re::Rm::Search_addr, ds_cap, 0);
  if (err) return -3;

  return 0;
}

void store_chunk(void *addr, Chunk &chunk)
{
  memcpy(addr, &chunk, sizeof(chunk));
}

void *malloc_init(size_t size)
{
  printf("init\n");
  static Dataspace app_ds;
  static Dataspace chunk_ds;
  static Dataspace link_ds;

  int err;

  err = allocate_dataspace(app_ds, size);
  if (err) return 0;
  err = allocate_dataspace(chunk_ds, G.min_allocate_size);
  if (err) return 0;
  err = allocate_dataspace(link_ds, G.min_allocate_size);
  if (err) return 0;

  app_ds.free = app_ds.size - size;


  Chunk *chunks = (Chunk*)chunk_ds.addr;
  LinkedChunk *links = (LinkedChunk*)link_ds.addr;

  Chunk &used = chunks[0];
  used.size = size;
  used.addr = app_ds.addr;

  LinkedChunk &free_link = links[0];
  free_link.addr = (void*)((size_t)app_ds.addr + size);
  free_link.next = NULL;

  Chunk &free = chunks[1];
  free.size = app_ds.free;
  free.addr = &free_link;
  free.free();


  G.biggest_free_chunk_size = free.size;
  G.free = &free;
  G.initialized = true;

  return used.addr;
}

void print_some()
{

}

void *malloc_allocate_and_use(size_t size)
{
  printf("allocate and use\n");
  /*Dataspace *ds = allocate_dataspace(size);
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
      size_t diff = free->size - size - 1;
      if (diff <= best_diff) {
	best = free;
	best_diff = diff;
	prev_best = prev;
	if (best_diff == 0) break;
      }
    }
    prev = free;
    free = free->free_next();
  }
}

void *malloc_find_and_use(size_t size)
{
  printf("find and use\n");

  Chunk *prev, *best;
  find_best_free_chunk(size, prev, best);
  /*
  if (chunk.size > size)
    split_chunk(chunk, size);
  use_chunk(chunk);
  return chunk.addr;
  */
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

  printf("malloc %i at %p", size, addr);
  print_some();
  printf("<<<<<<\n");

  return addr;
}

/*
void *malloc(size_t size) throw ()
{
  printf("malloc %i\n", size);
  L4::Cap<L4Re::Dataspace> ds
    = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  
  if (!ds.is_valid()) return 0;
  long err = L4Re::Env::env()->mem_alloc()->alloc(size, ds);
  if (err) return 0;
  void *addr = 0;
  err = L4Re::Env::env()->rm()
    ->attach(&addr, size, L4Re::Rm::Search_addr, ds, 0);
  if (err) return 0;
  return addr;
}
*/

void free(void *p) throw()
{
  enter_kdebug("free");
}
