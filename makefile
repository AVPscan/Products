#
# Copyright (C) 2026 Поздняков Алексей Васильевич
# E-mail: avp70ru@mail.ru
# 
# Данная программа является свободным программным обеспечением: вы можете 
# распространять ее и/или изменять согласно условиям Стандартной общественной 
# лицензии GNU (GPLv3).
#

CC ?= gcc
TARGET = food

UNAME_S := $(shell uname -s)

ifeq ($(OS),Windows_NT)
	SYS_SRC = sys_windows.c
	EXT = .exe
	LIBS = -lkernel32 -luser32
	GET_SIZE = wc -c < $(TARGET)$(EXT)
else ifeq ($(UNAME_S),Darwin)
	SYS_SRC = sys_macos.c
	EXT =
	LIBS =
	GET_SIZE = stat -f %z $(TARGET)$(EXT)
else
	SYS_SRC = sys_linux.c
	EXT =
	LIBS =
	GET_SIZE = stat -c%s $(TARGET)$(EXT)
endif

SOURCES = main.c engine.c $(SYS_SRC)
BASE_CFLAGS = -std=c11 -Os -DNDEBUG -Wall -Wextra -flto

ifneq ($(OS),Windows_NT)
	ifeq ($(UNAME_S),Linux)
		BASE_CFLAGS += -D_POSIX_C_SOURCE=200809L
	endif
endif

BASE_LDFLAGS = -flto $(LIBS)

ifeq ($(UNAME_S),Darwin)
	BASE_LDFLAGS += -Wl,-dead_strip
else ifeq ($(OS),Windows_NT)
	BASE_LDFLAGS += -Wl,--gc-sections -Wl,--strip-all -s
else
	BASE_LDFLAGS += -Wl,--gc-sections -Wl,--strip-all -Wl,-s -Wl,--build-id=none -Wl,-z,norelro -Wl,-z,pack-relative-relocs
endif

CFLAGS_TINY = $(BASE_CFLAGS) -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-ident -fomit-frame-pointer -fno-stack-protector
LDFLAGS_TINY = $(BASE_LDFLAGS)

.PHONY: all tiny clean run size help g c musl g-musl mac

all: tiny
tiny: $(SOURCES)
	@echo "🎯 Сборка: $(SYS_SRC) -> $(TARGET)$(EXT) ($(UNAME_S))"
	@$(CC) $(CFLAGS_TINY) -o $(TARGET)$(EXT) $(SOURCES) $(LDFLAGS_TINY)

	@if [ "$(OS)" != "Windows_NT" ] && [ "$(UNAME_S)" != "Darwin" ]; then strip --strip-all --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag \
		--remove-section=.comment --remove-section=.eh_frame --remove-section=.eh_frame_hdr $(TARGET)$(EXT) 2>/dev/null || true; \
	elif [ "$(UNAME_S)" = "Darwin" ]; then strip -x $(TARGET)$(EXT) 2>/dev/null || true; \
	elif [ "$(OS)" = "Windows_NT" ]; then strip --strip-all $(TARGET)$(EXT) 2>/dev/null || true; \
	fi
	@$(MAKE) --no-print-directory size

g: CC = gcc
g: tiny

c: CC = clang
c: CFLAGS_TINY += -Oz
c: tiny

musl: g-musl
g-musl: 
	@if [ "$(UNAME_S)" != "Linux" ]; then \
		echo "⚠️  MUSL static build is only supported on Linux environment."; \
		exit 1; \
	fi
	@$(MAKE) tiny CC=gcc CFLAGS_TINY="$(CFLAGS_TINY) -static" LDFLAGS_TINY="$(LDFLAGS_TINY) -static"
mac:
	@if [ "$(UNAME_S)" != "Darwin" ]; then echo "⚠️  'make mac' target only runs on macOS (Darwin).";
	else $(MAKE) tiny; fi
size:
	@SIZE=$$($(GET_SIZE) 2>/dev/null || echo 0); \
	echo "📏 Размер бинарника: $$SIZE байт"; \
	TARGET_SIZE=27000; \
	if [ $$SIZE -le $$TARGET_SIZE ] && [ $$SIZE -gt 0 ]; then \
	    echo "✅ Лимит выдержан"; \
	elif [ $$SIZE -gt 0 ]; then \
	    echo "⚠️  Превышение на $$((SIZE - TARGET_SIZE)) байт"; \
	fi
clean:
	rm -f $(TARGET) $(TARGET).exe
	@echo "🧹 Очищено"
