CC = clang

NAME = sbc

MK_PATH = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
MK_PARENT = $(realpath $(MK_PATH)../)

ifndef CONFIG
	CONFIG = Debug
endif

ifeq ($(CONFIG), Debug)
	OPTFLAGS = -O0 -g -DDEBUG
else
	OPTFLAGS = -Os -DNDEBUG
endif

FDIR = /Library/Frameworks

SRCDIR  = $(MK_PARENT)/source
IDIR    = $(MK_PARENT)/include
OBJDIR	= $(MK_PATH)bin

X86DIR	= $(OBJDIR)/x86_64
ARMDIR	= $(OBJDIR)/arm64

CSOURCE  = $(shell find $(SRCDIR) -type f -name *.c)
OCSOURCE = $(shell find $(SRCDIR) -type f -name *.m)

CFLAGS   = $(OPTFLAGS) -Wall -Wextra -Wpedantic -F $(FDIR) -I$(IDIR)
OCFLAGS  = $(CFLAGS) -fobjc-arc

LDFLAGS  = $(OPTFLAGS) -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk \
			-F $(FDIR) -I$(FDIR)/SDL2.framework/Headers \
			-I$(FDIR)/SDL2.framework/Headers -framework SDL2 \
			-framework CoreAudio -framework Cocoa -Wl,-rpath,$(FDIR) -lm 

X86OBJ := $(CSOURCE:$(SRCDIR)/%.c=$(X86DIR)/%.o) $(OCSOURCE:$(SRCDIR)/%.m=$(X86DIR)/%.o)
ARMOBJ := $(CSOURCE:$(SRCDIR)/%.c=$(ARMDIR)/%.o) $(OCSOURCE:$(SRCDIR)/%.m=$(ARMDIR)/%.o)

.PHONY: build compile clean make_bin compile_x86
build: make_bin compile

$(X86DIR)/%.o: $(SRCDIR)/%.m
	$(CC) $<  $(OCFLAGS) -arch x86_64 -target x86_64-apple-macos10.12 -c -o $@

$(X86DIR)/%.o: $(SRCDIR)/%.c
	$(CC) $<  $(CFLAGS) -arch x86_64 -target x86_64-apple-macos10.12 -c -o $@

$(ARMDIR)/%.o: $(SRCDIR)/%.m
	$(CC) $<  $(OCFLAGS) -arch arm64 -target arm64-apple-macos11 -c -o $@

$(ARMDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $<  $(CFLAGS) -arch arm64 -target arm64-apple-macos11 -c -o $@

$(NAME)_x86_64: $(X86OBJ)
	@rm -rf sbc
	$(CC) -o $@ $^ $(LDFLAGS) -arch x86_64 

$(NAME)_arm64: $(ARMOBJ)
	@rm -rf sbc
	$(CC) -o $@ $^ $(LDFLAGS) -arch arm64 

$(NAME):
	@lipo $(NAME)_x86_64 $(NAME)_arm64 -create -output $@

compile: $(NAME)_x86_64 $(NAME)_arm64 $(NAME)

ifneq ($(CONFIG),Debug)
	@rm -rf $(NAME)_x86_64 $(NAME)_arm64
endif

clean:
	@rm -f $(X86DIR)/*.o $(ARMDIR)/*.o $(OBJDIR)/*~ $(NAME)_x86_64 $(NAME)_arm64 $(NAME)

ifneq ($(wildcard $(ARMDIR)/.*),)
	@rmdir $(ARMDIR)
endif

ifneq ($(wildcard $(X86DIR)/.*),)
	@rmdir $(X86DIR)
endif

ifneq ($(wildcard $(OBJDIR)/.*),)
	@rmdir $(OBJDIR)
endif

make_bin:
ifeq ($(wildcard $(OBJDIR)/.*),)
	@mkdir $(OBJDIR)
endif

ifeq ($(wildcard $(ARMDIR)/.*),)
	@mkdir $(ARMDIR)
endif

ifeq ($(wildcard $(X86DIR)/.*),)
	@mkdir $(X86DIR)
endif
