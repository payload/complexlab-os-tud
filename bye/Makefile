PKGDIR          ?= .
L4DIR           ?= $(PKGDIR)/../..

TARGET                   = ex_bye-server ex_bye-client
REQUIRES_LIBS := libc_be_mem
SRC_CC_ex_bye-server = server.cc
SRC_CC_ex_bye-client = client.cc

include $(L4DIR)/mk/prog.mk

