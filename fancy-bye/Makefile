PKGDIR          ?= .
L4DIR           ?= $(PKGDIR)/../..

TARGET                   = fancy-bye-server fancy-bye-client
REQUIRES_LIBS := libc_be_mem libgfxbitmap
SRC_CC_fancy-bye-server = server.cc
SRC_CC_fancy-bye-client = client.cc

include $(L4DIR)/mk/prog.mk

