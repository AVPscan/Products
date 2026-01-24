#ifndef SYS_H
#define SYS_H

#include <stddef.h>
#include <stdarg.h>

#define USE_RGB_PALETTE // комментировать если нужно попроще не RGB цвет
#ifdef USE_RGB_PALETTE
  #define Cnn "\033[38;2;128;128;128m"
  #define Cna "\033[38;2;218;165;32m"
  #define Cpr "\033[38;2;50;205;50m"
  #define Cnu "\033[38;2;138;43;226m"
  #define Cap "\033[38;2;60;179;113m"
  #define Cam "\033[38;2;220;20;60m"
#else
  #define Cnn "\033[90m"
  #define Cna "\033[33m"
  #define Cpr "\033[32m"
  #define Cnu "\033[35m"
  #define Cap "\033[33m"
  #define Cam "\033[31m"
#endif
#define Crs   "\033[0m"

#define HCur  "\033[?25l"
#define ShCur "\033[?25h"
#define Cls   "\033[2J\033[H"
#define SCur  "\033[s"
#define LCur  "\033[u"
#define Cce   "\033[K"

#define DBuf 4096
#define NBuf 1024

/* --- Файловые операции --- */
void* os_open_file(const char* name);
void* os_create_file(const char* name);
void  os_close_file(void* handle);
int   os_read_file(void* handle, unsigned char* buf, int len);
int   os_print_file(void* handle, const char* format, ...);

/* --- Системные функции вывода --- */
void  os_printf(const char* format, ...);
int   os_snprintf(char* buf, size_t size, const char* format, ...);

//=== Рабочая директория ===
void SWD(void);

/* --- Управление памятью --- */
void* os_malloc(size_t size);
void* os_realloc(void* ptr, size_t size);
void  os_free(void* ptr);
void  os_memset(void* ptr, int val, size_t size);
char* os_strdup(const char* s);

/* --- Время и Задержки --- */
void  delay_ms(int ms);
void  CalibrateUIDelay(void);
void  UIDelay(int base_ms);

/* --- Ввод и Клавиатура --- */
void  SetInputMode(int raw);
const char* GetKeyName(void);

#endif /* SYS_H */
