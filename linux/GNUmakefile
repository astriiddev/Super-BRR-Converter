CC = gcc
NAME = sbc

MK_PATH   = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
MK_PARENT = $(realpath $(MK_PATH)../)

ifndef CONFIG
	CONFIG = Debug
endif

SRCDIR  = $(MK_PARENT)/source
IDIR    = $(MK_PARENT)/include
OBJDIR	= $(MK_PATH)bin

CSOURCE = $(shell find $(SRCDIR) -type f -name *.c)

CFLAGS	= -Wall -Wextra -Wpedantic -Winit-self -Werror -std=c11 -D_GNU_SOURCE \
		  $(shell sdl2-config --cflags) $(shell pkg-config --cflags gtk+-3.0) -I$(IDIR)

LDFLAGS = $(shell sdl2-config --libs) $(shell pkg-config --libs gtk+-3.0) -lm -latomic

ifeq ($(CONFIG), Debug)
CFLAGS  += -O0 -g -DDEBUG
else ifeq ($(CONFIG), Release)
CFLAGS  += -O2 -DNDEBUG
LDFLAGS += -s
endif

OBJS   := $(CSOURCE:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: build compile clean make_bin check_xlib
build: make_bin check_xlib compile

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
