# ============================================
# Makefile для достижения 27KB
# С правильными POSIX флагами
# ============================================

CC ?= gcc
TARGET = products
SOURCES = parser.c sys_linux.c

# Флаги для минимального размера
CFLAGS_TINY = -std=c11 -Os -DNDEBUG -Wall -Wextra \
              -D_POSIX_C_SOURCE=200809L \
              -ffunction-sections -fdata-sections \
              -fno-stack-protector -fomit-frame-pointer \
              -fno-unwind-tables -fno-asynchronous-unwind-tables \
              -fno-ident

LDFLAGS_TINY = -Wl,--gc-sections -Wl,--strip-all -Wl,-s \
               -Wl,--build-id=none -Wl,-z,norelro

.PHONY: all tiny clean run size analyze help

# По умолчанию - попытка достичь 27KB
all: tiny

# Основная цель - 27KB
tiny: $(SOURCES)
	@echo "🎯 Цель: 27KB бинарник с nanosleep..."
	$(CC) $(CFLAGS_TINY) -o $(TARGET) $(SOURCES) $(LDFLAGS_TINY)
	@# Дополнительный стрип
	@strip --strip-all --remove-section=.note.gnu.build-id \
	       --remove-section=.comment $(TARGET) 2>/dev/null || true
	@$(MAKE) --no-print-directory size

# Альтернатива без nanosleep для сравнения
original: CFLAGS_TINY = -std=c11 -Os -DNDEBUG -Wall -Wextra \
                        -ffunction-sections -fdata-sections \
                        -fno-stack-protector -fomit-frame-pointer
original: LDFLAGS_TINY = -Wl,--gc-sections -Wl,--strip-all -Wl,-s
original: $(SOURCES)
	@echo "🔄 Сборка с оригинальной delay_ms (без nanosleep)..."
	$(CC) $(CFLAGS_TINY) -o $(TARGET) $(SOURCES) $(LDFLAGS_TINY)
	@strip --strip-all $(TARGET) 2>/dev/null || true
	@$(MAKE) --no-print-directory size

# Показать размер
size:
	@SIZE=$$(stat -c%s $(TARGET) 2>/dev/null || wc -c < $(TARGET)); \
	echo "📏 Размер: $$SIZE байт"; \
	if [ $$SIZE -le 28000 ]; then \
	    if [ $$SIZE -le 27000 ]; then \
	        echo "✅ ЦЕЛЬ ДОСТИГНУТА: ≤27KB!"; \
	    else \
	        echo "⚠️  Близко: $$((SIZE - 27000)) байт сверху"; \
	    fi; \
	else \
	    echo "❌ Большой: $$((SIZE - 27000)) байт лишних"; \
	fi

# Анализ
analyze: $(TARGET)
	@echo "🔍 Анализ бинарника..."
	@echo "1. Размер:"
	@ls -lh $(TARGET) | awk '{print "   "$$5" ("$$9")"}'
	@echo ""
	@echo "2. Зависимости:"
	@ldd $(TARGET) 2>/dev/null || echo "   (статический или ошибка)"
	@echo ""
	@echo "3. Секции:"
	@size $(TARGET) 2>/dev/null || echo "   (size не доступен)"

# Разные компиляторы
g: CC = gcc
g: tiny

c: CC = clang
c: CFLAGS_TINY += -Oz -flto -fvisibility=hidden
c: tiny

# Очистка
clean:
	rm -f $(TARGET) *.o
	@echo "🧹 Очищено"

# Запуск
run: tiny
	@echo "🚀 Запуск программы..."
	@echo "========================"
	./$(TARGET)

# Тест скорости
bench: tiny
	@echo "⏱️  Тест скорости..."
	@time ./$(TARGET) --help 2>/dev/null || echo "   (без --help опции)"
	@echo "   Запуск теста завершен"

# Справка
help:
	@echo "=== Makefile для продуктов ==="
	@echo "Команды для достижения 27KB:"
	@echo "  make           - Сборка с nanosleep (цель 27KB)"
	@echo "  make original  - Сборка без nanosleep (сравнение)"
	@echo "  make size      - Показать размер"
	@echo "  make analyze   - Анализ бинарника"
	@echo "  make g		- Собрать с GCC"
	@echo "  make c		- Собрать с Clang"
	@echo "  make clean     - Очистка"
	@echo "  make run       - Собрать и запустить"
	@echo "  make bench     - Тест скорости"
	@echo ""
	@echo "Текущий компилятор: $(CC)"
