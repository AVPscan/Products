CC ?= gcc
TARGET = products
SOURCES = products.c sys_linux.c

# Базовые флаги
BASE_CFLAGS = -std=c11 -Os -DNDEBUG -Wall -Wextra -D_POSIX_C_SOURCE=200809L
BASE_LDFLAGS = -flto -Wl,--gc-sections -Wl,--strip-all -Wl,-s -Wl,--build-id=none

# Флаги для tiny версии
CFLAGS_TINY = $(BASE_CFLAGS) \
              -ffunction-sections -fdata-sections \
              -fno-unwind-tables -fno-asynchronous-unwind-tables \
              -fno-ident -fomit-frame-pointer

LDFLAGS_TINY = $(BASE_LDFLAGS) \
               -Wl,-z,pack-relative-relocs

.PHONY: all tiny clean run size analyze help g c

all: tiny

tiny: $(SOURCES)
	@echo "🎯 Цель: минимальный бинарник..."
	$(CC) $(CFLAGS_TINY) -o $(TARGET) $(SOURCES) $(LDFLAGS_TINY)
	@# Дополнительный strip на всякий случай
	@strip --strip-all --remove-section=.note.gnu.build-id \
	       --remove-section=.note.ABI-tag \
	       --remove-section=.comment $(TARGET) 2>/dev/null || true
	@$(MAKE) --no-print-directory size

# Специально для GCC
g: CC = gcc
g: tiny

# Специально для Clang (используем -Oz для максимального сжатия)
c: CC = clang
c: CFLAGS_TINY += -Oz -fno-stack-protector -fno-unwind-tables
c: tiny


# Для сравнения - обычная сборка
normal:
	@echo "🔨 Обычная сборка..."
	$(CC) $(BASE_CFLAGS) -o $(TARGET) $(SOURCES)
	@$(MAKE) --no-print-directory size

size:
	@SIZE=$$(stat -c%s $(TARGET) 2>/dev/null || wc -c < $(TARGET)); \
	echo "📏 Размер: $$SIZE байт"; \
	TARGET_SIZE=27000; \
	if [ $$SIZE -le $$TARGET_SIZE ]; then \
	    echo "✅ мы сделали это однако"; \
	else \
	    echo "⚠️  Размер: $$SIZE байт (превышение на $$((SIZE - TARGET_SIZE)))"; \
	fi

$(TARGET): $(SOURCES)
	$(MAKE) tiny
analyze: $(TARGET)
	@echo "🔍 Анализ секций:"
	@size $(TARGET)
	@echo "🔍 Подробно:"
	@size -A $(TARGET) 2>/dev/null || echo "size -A не поддерживается"
	@echo "🔍 Динамические зависимости:"
	@ldd $(TARGET) 2>/dev/null || echo "Статически слинкован"

clean:
	rm -f $(TARGET) *.o
	@echo "🧹 Очищено"

run: tiny
	./$(TARGET)

help:
	@echo "Доступные цели:"
	@echo "  make tiny   - минимальная сборка (по умолчанию)"
	@echo "  make g      - tiny сборка через gcc"
	@echo "  make c      - tiny сборка через clang с -Oz"
	@echo "  make normal - обычная сборка для сравнения"
	@echo "  make size   - показать размер бинарника"
	@echo "  make analyze- анализ секций и зависимостей"
	@echo "  make clean  - очистка"
	@echo "  make run    - собрать и запустить"
