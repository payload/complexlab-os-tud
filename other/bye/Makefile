PKGDIR          ?= .
L4DIR           ?= $(PKGDIR)/../..

TARGET                   = bye-server bye-client
REQUIRES_LIBS := libc_be_mem
SRC_CC_bye-server = server.cc
SRC_CC_bye-client = client.cc

include $(L4DIR)/mk/prog.mk

