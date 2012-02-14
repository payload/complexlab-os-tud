/**
 * \file   libc_backends/l4re_mem/mem.cc
 */
/*
 * (c) 2004-2009 Technische Universität Dresden
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */
#include <stdlib.h>
#include <l4/sys/kdebug.h>

#include <l4/re/util/cap_alloc>
#include <l4/re/dataspace>
#include <l4/re/env>

void *malloc_allocate_and_use(unsigned size);
void *malloc_find_and_use(unsigned size);

void *malloc(unsigned size)
{
  if (biggest_free_chunk < size)
    return malloc_allocate_and_use(size);
  else
    return malloc_find_and_use(size);
}

void *malloc_allocate_and_use(unsigned size)
{
}

  /*
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
  */


void free(void *p) throw()
{
  enter_kdebug("free");
}
