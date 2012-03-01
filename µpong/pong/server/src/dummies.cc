#include "dummies.h"

#include <l4/re/env>
#include <l4/re/namespace>

void pong_names_register( l4_cap_idx_t id, char const *name )
{
  L4Re::Env::env()->names()->register_obj(name, L4::Cap<void>(id));
}

