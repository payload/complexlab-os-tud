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

struct Dataspace {
  void *addr;
  size_t size;
  size_t free;
  Dataspace *next;
};

struct Chunk {
  size_t size;
  Chunk *next;
};

struct malloc_state {
  bool initialized;
  size_t allocate_size_step;
  size_t min_allocate_size;
  size_t biggest_free_chunk_size;
  Dataspace *ds_head;
  Chunk *used_head;
  Chunk *free_head;
} malloc_state = {
  false,
  L4_PAGESIZE,
  L4_PAGESIZE,
  0,
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

  if (size < malloc_state.min_allocate_size)
    ds.size =
      malloc_state.min_allocate_size;
  else
    ds.size =
      malloc_state.allocate_size_step *
      ((size / malloc_state.allocate_size_step) + 1);

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
  static Dataspace ds;

  // allocate at least enough to have some bytes free afterwards
  int err = allocate_dataspace(ds, size + 2*sizeof(Chunk) + 1);
  if (err) return 0;

  Chunk free = { ds.size - 2*sizeof(Chunk) - size, NULL };
  store_chunk(ds.addr, free);
  Chunk used = { size, NULL };
  void *used_addr = (void*)((size_t)ds.addr + ds.size - sizeof(Chunk) - size);
  store_chunk(used_addr, used);
  ds.free = ds.size - 2*sizeof(Chunk);

  malloc_state.ds_head = &ds;
  malloc_state.free_head = &free;
  malloc_state.biggest_free_chunk_size = free.size;

  return (void*)((size_t)used_addr + sizeof(used));
}

void *malloc(size_t size) throw ()
{
  printf("malloc %i at ", size);
  void *addr;
  if (malloc_state.biggest_free_chunk_size > size)
    addr = malloc_find_and_use(size);
  else if (!malloc_state.free_head)
    addr = malloc_init(size);
  else
    malloc_allocate_and_use(size);
  printf("%p\n", addr);
  return addr;
}

void *malloc_allocate_and_use(size_t size)
{
  /*  Dataspace *ds = allocate_dataspace(size);
  if (!ds) return 0;
  Chunk &chunk = create_free_chunk(ds, size);
  use_chunk(chunk);
  return chunk.addr;
  */
  return 0;
}

void *malloc_find_and_use(size_t size)
{
  /*
  Chunk &chunk = find_best_free_chunk(size);
  if (chunk.size > size)
    split_chunk(chunk, size);
  use_chunk(chunk);
  return chunk.addr;
  */
  return 0;
}

Dataspace *create_dataspace(size_t size)
{
  return 0;
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
