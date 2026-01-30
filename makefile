CC ?= gcc
TARGET = products
# Определяем ОС: Windows_NT — это стандартная переменная окружения в Win
ifeq ($(OS),Windows_NT)
    SYS_SRC = sys_windows.c
    EXT = .exe
    # Библиотеки для WinAPI
    LIBS = -lkernel32 -luser32
    # В Windows stat -c%s не работает, используем wc
    GET_SIZE = wc -c < $(TARGET)$(EXT)
else
    SYS_SRC = sys_linux.c
    EXT =
    LIBS =
    GET_SIZE = stat -c%s $(TARGET)$(EXT)
endif

SOURCES = products.c $(SYS_SRC)

# Базовые флаги
BASE_CFLAGS = -std=c11 -Os -DNDEBUG -Wall -Wextra
ifndef ($(OS),Windows_NT)
    BASE_CFLAGS += -D_POSIX_C_SOURCE=200809L
endif

BASE_LDFLAGS = -flto -Wl,--gc-sections -Wl,--strip-all -Wl,-s -Wl,--build-id=none $(LIBS)

# Флаги для tiny версии
CFLAGS_TINY = $(BASE_CFLAGS) \
              -ffunction-sections -fdata-sections \
              -fno-unwind-tables -fno-asynchronous-unwind-tables \
              -fno-ident -fomit-frame-pointer

LDFLAGS_TINY = $(BASE_LDFLAGS)
ifneq ($(OS),Windows_NT)
    LDFLAGS_TINY += -Wl,-z,pack-relative-relocs
endif

.PHONY: all tiny clean run size analyze help g c

all: tiny

tiny: $(SOURCES)
	@echo "🎯 Цель: минимальный бинарник ($(SYS_SRC))..."
	$(CC) $(CFLAGS_TINY) -o $(TARGET)$(EXT) $(SOURCES) $(LDFLAGS_TINY)
	@# Strip для Linux (в Windows gcc делает это сам при -s)
	@if [ "$(OS)" != "Windows_NT" ]; then \
	    strip --strip-all --remove-section=.note.gnu.build-id \
	          --remove-section=.note.ABI-tag \
	          --remove-section=.comment $(TARGET)$(EXT) 2>/dev/null || true; \
	fi
	@$(MAKE) --no-print-directory size

g: CC = gcc
g: tiny

c: CC = clang
c: CFLAGS_TINY += -Oz -fno-stack-protector
c: tiny

size:
	@SIZE=$$($(GET_SIZE) 2>/dev/null || echo 0); \
	echo "📏 Размер: $$SIZE байт"; \
	TARGET_SIZE=27000; \
	if [ $$SIZE -le $$TARGET_SIZE ] && [ $$SIZE -gt 0 ]; then \
	    echo "✅ мы сделали это однако"; \
	elif [ $$SIZE -gt 0 ]; then \
	    echo "⚠️  Размер: $$SIZE байт (превышение на $$((SIZE - TARGET_SIZE)))"; \
	fi

clean:
	rm -f $(TARGET) $(TARGET).exe *.o
	@echo "🧹 Очищено"

run: tiny
	./$(TARGET)$(EXT)

help:
	@echo "ОС: $(OS) (Файл: $(SYS_SRC))"
	@echo "Доступные цели: tiny, g, c, clean, run"

