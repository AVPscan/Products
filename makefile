#
# Copyright (C) 2026 Поздняков Алексей Васильевич
# E-mail: avp70ru@mail.ru
# 
# Данная программа является свободным программным обеспечением: вы можете 
# распространять ее и/или изменять согласно условиям Стандартной общественной 
# лицензии GNU (GPLv3).
#

CC ?= gcc
TARGET = products

# Автоматически определяем ОС, если переменная OS не задана
UNAME_S := $(shell uname -s)

ifeq ($(OS),Windows_NT)
	SYS_SRC = sys_windows.c
	EXT = .exe
	LIBS = -lkernel32 -luser32
	GET_SIZE = wc -c < $(TARGET)$(EXT)
else ifeq ($(UNAME_S),Darwin)
	# Конфигурация для macOS (Darwin)
	SYS_SRC = sys_macos.c
	EXT =
	LIBS =
	GET_SIZE = stat -f %z $(TARGET)$(EXT)
else
	# Конфигурация по умолчанию для Linux
	SYS_SRC = sys_linux.c
	EXT =
	LIBS =
	GET_SIZE = stat -c%s $(TARGET)$(EXT)
endif

SOURCES = products.c $(SYS_SRC)

# Базовые флаги
BASE_CFLAGS = -std=c11 -Os -DNDEBUG -Wall -Wextra

# ИСПРАВЛЕНИЕ: Добавляем POSIX флаг ТОЛЬКО для Linux, не для Windows и не для Darwin (macOS)
ifneq ($(OS),Windows_NT)
	ifeq ($(UNAME_S),Linux)
		BASE_CFLAGS += -D_POSIX_C_SOURCE=200809L
	endif
endif

# Флаги линковки
BASE_LDFLAGS = -flto -Wl,--gc-sections -Wl,--strip-all -Wl,-s -Wl,--build-id=none $(LIBS)

CFLAGS_TINY = $(BASE_CFLAGS) \
			  -ffunction-sections -fdata-sections \
			  -fno-unwind-tables -fno-asynchronous-unwind-tables \
			  -fno-ident -fomit-frame-pointer

LDFLAGS_TINY = $(BASE_LDFLAGS)
ifneq ($(OS),Windows_NT)
	LDFLAGS_TINY += -Wl,-z,pack-relative-relocs
endif

.PHONY: all tiny clean run size help g c musl g-musl

all: tiny

tiny: $(SOURCES)
	@echo "🎯 Сборка: $(SYS_SRC) -> $(TARGET)$(EXT) ($(UNAME_S))"
	@$(CC) $(CFLAGS_TINY) -o $(TARGET)$(EXT) $(SOURCES) $(LDFLAGS_TINY)
	@if [ "$(OS)" != "Windows_NT" ] && [ "$(UNAME_S)" != "Darwin" ]; then strip --strip-all --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag --remove-section=.comment $(TARGET)$(EXT) 2>/dev/null || true; elif [ "$(UNAME_S)" = "Darwin" ]; then strip -x $(TARGET)$(EXT) 2>/dev/null || true; fi
	@$(MAKE) --no-print-directory size

g: CC = gcc
g: tiny

c: CC = clang
c: CFLAGS_TINY += -Oz -fno-stack-protector
c: tiny

# Новая цель: Статическая сборка MUSL (Только для Linux)
musl: g-musl

g-musl: 
	@if [ "$(UNAME_S)" != "Linux" ]; then echo "⚠️  MUSL static build is only supported on Linux environment."; else $(MAKE) tiny CC=gcc CFLAGS_TINY="$(CFLAGS_TINY) -static" LDFLAGS_TINY="$(LDFLAGS_TINY) -static"; fi

size:
	@SIZE=$$(stat -c%s $(TARGET) 2>/dev/null || echo 0); \
	echo "📏 Размер бинарника: $$SIZE байт"; \
	TARGET_SIZE=27000; \
	if [ $$SIZE -le $$TARGET_SIZE ] && [ $$SIZE -gt 0 ]; then \
	    echo "✅ Лимит выдержан (до 27КБ)"; \
	elif [ $$SIZE -gt 0 ]; then \
	    echo "⚠️  Превышение на $$((SIZE - TARGET_SIZE)) байт"; \
	fi

clean:
	rm -f $(TARGET) $(TARGET).exe
	@echo "🧹 Очищено"

run: tiny
	./$(TARGET)$(EXT)

help:
	@echo "Система: $(OS) | UNAME: $(UNAME_S) | Модуль: $(SYS_SRC)"
	@echo "Цели: tiny (default), g (gcc), c (clang), run, clean, musl (static musl build on Linux only)"
