L4DIR ?=
ifndef L4DIR
  $(error Set L4DIR in this file or as an environment variable)
endif
L4DIR := $(shell cd $(L4DIR); pwd)

O ?=
ifndef O
  $(error Set O in this file or as an evironment variable)
endif
O := $(shell cd $(O); pwd)

QEMU_OPTIONS += -serial stdio -sdl

export
µpong.lua = $(L4DIR)/conf/µpong.lua
mem.cc-path = $(L4DIR)/pkg/libc_backends/lib/l4re_mem/
mem.cc = $(mem.cc-path)/mem.cc
modules.list = $(L4DIR)/conf/modules.list

µpong: fast-clean
	ln -s `pwd`/µpong/µpong.lua $(µpong.lua)
	ln -s `pwd`/mem.cc $(mem.cc)
	cat modules.entry >> $(modules.list)
	$(MAKE) -C $(mem.cc-path)
	$(MAKE) -C µpong
	$(MAKE) -C $(O) qemu

fast-clean:
	rm -f $(mem.cc) $(µpong.lua)
	sed -i '/^entry µpong/,/^#end entry µpong/d' $(modules.list)

clean: fast-clean
	$(MAKE) -C µpong clean
