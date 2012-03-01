PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

vpath %.bmp ..

MODE		= static
#MODE		= shared
TARGET		= pong-client
SRC_CC		= main.cc
LIBS           += -lcxx_thread_e -lcxx_libc_io

REQUIRES_LIBS += libc_be_mem libpthread cxx_io cxx_base libstdc++

include $(L4DIR)/mk/prog.mk
