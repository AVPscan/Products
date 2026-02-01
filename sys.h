/*
 * Copyright (C) 2026 Поздняков Алексей Васильевич
 * E-mail: avp70ru@mail.ru
 * 
 * Данная программа является свободным программным обеспечением: вы можете 
 * распространять ее и/или изменять согласно условиям Стандартной общественной 
 * лицензии GNU (GPLv3).
 */
 
#ifndef SYS_H
#define SYS_H

#include <stddef.h>
#include <stdarg.h>

//#define USE_BW
#define USE_RGB

#ifdef USE_BW
  #define Cnn ""
  #define Cna ""
  #define Cpr ""
  #define Cnu ""
  #define Cap ""
  #define Cam ""
#else
  #ifdef USE_RGB
    #define Cnn "\033[38;2;120;120;120m"
    #define Cna "\033[38;2;210;105;30m"
    #define Cpr "\033[38;2;184;134;11m"
    #define Cnu "\033[38;2;30;144;255m"
    #define Cap "\033[38;2;34;139;34m"
    #define Cam "\033[38;2;220;20;60m"
  #else // обычные
    #define Cnn "\033[38;5;244m" // Номер (Серый)
    #define Cna "\033[38;5;166m" // Имя (Оранжевый)
    #define Cpr "\033[38;5;178m" // Цена (Золотистый) — хорошо выделяется
    #define Cnu "\033[38;5;27m"  // Число (Синий)
    #define Cap "\033[38;5;28m"  // Рост/Плюс (Темно-зеленый)
    #define Cam "\033[38;5;160m" // Падение/Минус (Красный)
  #endif
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

enum {
    K_ESC = 1, K_UP, K_DOW, K_RIG, K_LEF, 
    K_HOM, K_END, K_PUP, K_PDN, K_INS, K_DEL,
    K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12,
    K_SPA, K_BAC, K_ENT, K_TAB, K_CRC };
    
void* os_open_file(const char* name);
void* os_create_file(const char* name);
void  os_close_file(void* handle);
int   os_read_file(void* handle, unsigned char* buf, int len);
int   os_read_file_at(void* handle, long offset, unsigned char* buf, int len);
int   os_print_file(void* handle, const char* format, ...);
void* os_malloc(size_t size);
void* os_realloc(void* ptr, size_t size);
void  os_free(void* ptr);
void  os_memset(void* ptr, int val, size_t size);
char* os_strdup(const char* s);
void  os_printf(const char* format, ...);
int   os_snprintf(char* buf, size_t size, const char* format, ...);
//=== Рабочая директория ===
void SWD(void);
/* --- Время и Задержки --- */
void  delay_ms(int ms);
/* --- Ввод и Клавиатура --- */
void  SetInputMode(int raw);
const char* GetKey(void);

int AutoEncryptOrValidate(const char *fname);
int SendMailSecure(const char *fname, const char *target);
#endif /* SYS_H */
