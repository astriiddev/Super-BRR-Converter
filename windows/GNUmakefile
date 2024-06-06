ifneq (,$(findstring clang64, $(shell where cc)))
CC  = clang
ENV = llvm
else
CC  = gcc
ENV = gnu
endif

ifeq ($(CC), clang)
ENV = llvm
endif

NAME = sbc

MK_PATH = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
MK_PARENT = $(realpath $(MK_PATH)../)

ifndef CONFIG
	CONFIG = Debug
endif

SRCDIR  = $(MK_PARENT)/source
IDIR    = $(MK_PARENT)/include
OBJDIR	= $(MK_PATH)bin

CSOURCE = $(shell ls $(SRCDIR)/*.c)

CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 $(shell sdl2-config --cflags) -I$(IDIR)
LDFLAGS = $(shell sdl2-config --libs) -lm -latomic -lmingw32 -ldwmapi -luuid -lwindowsapp

ifeq ($(CONFIG), Debug)
CFLAGS  += -O0 -g -D_DEBUG -D_MT -DDEBUG -D__WIN_DEBUG__ -D_CRTDBG_MAP_ALLOC
LDFLAGS += -mconsole
else ifeq ($(CONFIG), Release)
CFLAGS  += -O2 -DNDEBUG
LDFLAGS += -mwindows -s
endif

ifeq ($(ENV), llvm)
	LDFLAGS += -luser32 -lshell32
endif

OBJS := $(CSOURCE:$(SRCDIR)/%.c=$(OBJDIR)/%.o) $(MK_PARENT)/res/sbc.o

.PHONY: build compile clean make_bin
build: make_bin compile

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $<  $(CFLAGS) -c -o $@

$(NAME): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

compile: $(NAME)

clean:
	@rm -f $(OBJDIR)/*.o $(OBJDIR)/*~ $(NAME)

ifneq ($(wildcard $(OBJDIR)/.*),)
	@rmdir $(OBJDIR)
endif

make_bin:
ifeq ($(wildcard $(OBJDIR)/.*),)
	@mkdir $(OBJDIR)
endif
