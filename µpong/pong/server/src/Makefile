PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

#MODE		= shared
MODE		= static
TARGET		= pong-server

OBJS		+= ball.img.o paddle.img.o

SYSTEMS		= x86-l4f amd64-l4f

SRC_CC		= main.cc gfx-drv.cc ball.cc env.cc sintab.cc paddle.cc \
                  paddle-server.cc
CXXFLAGS	+= -Wall -Wreorder 

LIBS            += -lcxx_thread_e -lcxx_libc_io -lcxx_io

REQUIRES_LIBS := libc_be_mem libstdc++ libpthread

include $(L4DIR)/mk/prog.mk

CHECKCXX        = $(shell if $(CXX) $(1) -S -o /dev/null -xc++ /dev/null \
		    > /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi ;)

CXXFLAGS	+= $(call CHECKCXX,-fno-threadsafe-statics,)

%.img.o: %.img
	$(OBJCOPY) -B $(BFD_ARCH) -I binary -O $(OFORMAT) $< $@

paddle.img: $(SRC_DIR)/paddle.bmp
	tail -c 3600 $< >$@

ball.img: $(SRC_DIR)/ball.bmp
	tail -c 1200 $< >$@

#%.img: $(SRC_DIR)/%.bmp
#	convert $< rgb:$@

