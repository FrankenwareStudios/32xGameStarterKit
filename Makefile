ifdef $(GENDEV)
ROOTDIR = $(GENDEV)
else
ROOTDIR = /opt/toolchains/sega
endif

LDSCRIPTSDIR = $(ROOTDIR)/ldscripts

LIBPATH = -L$(ROOTDIR)/sh-elf/lib -L$(ROOTDIR)/sh-elf/lib/gcc/sh-elf/12.1.0 -L$(ROOTDIR)/sh-elf/sh-elf/lib
INCPATH = -I. -I$(ROOTDIR)/sh-elf/include -I$(ROOTDIR)/sh-elf/sh-elf/include -I./liblzss

CCFLAGS = -c -std=c11 -g -m2 -mb
CCFLAGS += -Wall -Wextra -pedantic -Wno-unused-parameter -Wimplicit-fallthrough=0 -Wno-missing-field-initializers -Wnonnull
CCFLAGS += -D__32X__ -DMARS
LDFLAGS = -T $(LDSCRIPTSDIR)/mars.ld -Wl,-Map=output.map -nostdlib -Wl,--gc-sections --specs=nosys.specs
ASFLAGS = --big

MARSHWCFLAGS := $(CCFLAGS)
MARSHWCFLAGS += -fno-lto

release: CCFLAGS += -Os -fomit-frame-pointer -ffast-math -funroll-loops -fno-align-loops -fno-align-jumps -fno-align-labels
release: CCFLAGS += -ffunction-sections -fdata-sections -flto
release: MARSHWCFLAGS += -O1
release: LDFLAGS += -flto

debug: CCFLAGS += -O1 -ggdb -fno-omit-frame-pointer
debug: MARSHWCFLAGS += -O1 -ggdb -fno-omit-frame-pointer

PREFIX = $(ROOTDIR)/sh-elf/bin/sh-elf-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJC = $(PREFIX)objcopy

DD = dd
RM = rm -f

TARGET = NewGame32x
LIBS = $(LIBPATH) -lc -lgcc -lgcc-Os-4-200 -lnosys
OBJS = \
	crt0.o \
	yatssd/draw.o \
	yatssd/dtiles.o \
	yatssd/dsprite.o \
	gamesupport.o \
	assets.o \
	dsprite_extensions.o \
	marssound.o \
	marshw.o \
	marsonly.o \
	marsnew.o \
	sh2_fixed.o \
	sh2_draw.o \
	sh2_mixer.o \
	lzss.o \
	font.o \
	game_engine.o \
	npc.o \
	main_character.o \
	collision.o
	

release: $(TARGET).32x

debug: $(TARGET).32x

all: release

m68k.bin:
	make -C src-md

$(TARGET).32x: $(TARGET).elf
	$(OBJC) -O binary $< temp.bin
	$(DD) if=temp.bin of=$@ bs=128K conv=sync

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET).elf

crt0.o: crt0.s m68k.bin

marshw.o: marshw.c
	$(CC) $(MARSHWCFLAGS) $(INCPATH) $< -o $@

%.o: %.c
	$(CC) $(CCFLAGS) $(INCPATH) $< -o $@

%.o: liblzss/%.c
	$(CC) $(CCFLAGS) $(INCPATH) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $(INCPATH) $< -o $@

clean:
	make clean -C src-md
	$(RM) *.o mr8k.bin $(TARGET).32x $(TARGET).elf output.map temp.bin temp2.bin
