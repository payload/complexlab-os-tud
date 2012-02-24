PKGDIR ?= .
L4DIR ?= $(PKGDIR)/../..

TARGET = hacky hacky-test fancy fancy-test
REQUIRES_LIBS := libc_be_mem libio-io libpthread libstdc++ libgfxbitmap
SRC_CC_hacky = hacky.cc
SRC_CC_hacky-test = hacky-test.cc
SRC_CC_fancy = fancy.cc
SRC_CC_fancy-test = fancy-test.cc

include $(L4DIR)/mk/prog.mk
