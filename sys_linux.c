#define _POSIX_C_SOURCE 200809L
#include <stdio.h>      // Для fopen, fread, vfprintf, FILE
#include <stdlib.h>     // Для malloc, realloc, free
#include <string.h>     // Для strlen, memset, memcpy
#include <stdarg.h>     // Для va_list, va_start
#include <time.h>       // Для nanosleep
#include <sys/time.h>   // Для gettimeofday
#include <termios.h>    // Для tcgetattr
#include <fcntl.h>      // Для fcntl
#include <unistd.h>     // Для read, close

#include "sys.h"

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

void* os_open_file(const char* name) { return (void*)fopen(name, "rb"); }
void* os_create_file(const char* name) { return (void*)fopen(name, "wb"); }
void os_close_file(void* handle) { if (handle) fclose((FILE*)handle); }

int os_read_file(void* handle, unsigned char* buf, int len) {
    if (!handle) return 0;
    return (int)fread(buf, 1, len, (FILE*)handle); }
    
int os_print_file(void* handle, const char* format, ...) {
    if (!handle) return 0;
    va_list args;
    va_start(args, format);
    int res = vfprintf((FILE*)handle, format, args);
    va_end(args);
    return res; }

int os_snprintf(char* buf, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int res = vsnprintf(buf, size, format, args);
    va_end(args);
    return res; }

void os_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args); }
    
//=== Рабочая директория ===
unsigned char FileBuf[DBuf+NBuf];
void SWD(void) {
    char *path = (char *)FileBuf;
    ssize_t len = readlink("/proc/self/exe", path, DBuf - 1);
    if (len <= 0) return;
    path[len] = '\0';
    if (strncmp(path, "/nix/store", 10) == 0) { const char *home = getenv("HOME"); if (home != NULL) chdir(home);
                                                return; }
    for (char *p = path + len; p > path; p--) if (*p == '/') { *p = '\0'; chdir(path); break; } }
    
// Память
void* os_malloc(size_t size) { return malloc(size); }
void* os_realloc(void* ptr, size_t size) { return realloc(ptr, size); }
void  os_free(void* ptr) { free(ptr); }
void  os_memset(void* ptr, int val, size_t size) { memset(ptr, val, size); }

char* os_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* d = (char*)os_malloc(len);
    if (d) memcpy(d, s, len);
    return d; }

//--Тестирование и установка адекватных задержек
int ui_delay_factor = 0;
void delay_ms(int ms) {
    if (ms > 0) { struct timespec ts; ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L; nanosleep(&ts, NULL); } }

void CalibrateUIDelay(void) {
    if (ui_delay_factor) return;
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL); delay_ms(100); gettimeofday(&tv2, NULL);
    long long diff_us = (tv2.tv_sec - tv1.tv_sec) * 1000000LL + (tv2.tv_usec - tv1.tv_usec);
    int measured_ms = (int)(diff_us / 1000LL);
    if (measured_ms!=0 && measured_ms >= 50 && measured_ms <= 150) { ui_delay_factor = 1000 / measured_ms;
        if (ui_delay_factor < 8) ui_delay_factor = 8;
        if (ui_delay_factor > 25) ui_delay_factor = 25; } 
    else ui_delay_factor = 12; }

void UIDelay(int base_ms) {
    if (base_ms <= 0) return;
    if (ui_delay_factor == 0) CalibrateUIDelay();
    int calibrated = base_ms * ui_delay_factor / 10;
    if (calibrated < 1) calibrated = 1;
    if (calibrated > 500) calibrated = 500;
    delay_ms(calibrated); }
    
//=== Ввод/клавиатура ===
void SetInputMode(int raw) {
    static struct termios oldt;
    if (raw) {
        tcgetattr(0, &oldt);
        struct termios newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO | ISIG);
        tcsetattr(0, TCSANOW, &newt);
        fcntl(0, F_SETFL, O_NONBLOCK); } 
    else { tcsetattr(0, TCSANOW, &oldt);
        fcntl(0, F_SETFL, 0); } } 

typedef struct { const char *seq; const char *name; } KeyMap;
KeyMap key_table[] = {
    {"[A", "Up"}, {"[B", "Down"}, {"[C", "Right"}, {"[D", "Left"},
    {"[H", "Home"}, {"[F", "End"}, {"[1~", "Home"}, {"[4~", "End"},
    {"[2~", "Insert"}, {"[3~", "Delete"}, {"[5~", "Page Up"}, {"[6~", "Page Down"},
    {"OP", "F1"}, {"OQ", "F2"}, {"OR", "F3"}, {"OS", "F4"},
    {"[15~", "F5"}, {"[17~", "F6"}, {"[18~", "F7"}, {"[19~", "F8"},
    {"[20~", "F9"}, {"[21~", "F10"}, {"[23~", "F11"}, {"[24~", "F12"} };
const char *GetKeyName(void) {
    static char out_buf[16];
    unsigned char c;
    if (read(0, &c, 1) <= 0) return "";
    if (c >= 0x80) {
        out_buf[0] = c;
        int len = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
        int i = 0;
        while (i < len)  i += read(0, &out_buf[i + 1], len - i);
        out_buf[i + 1] = '\0';
        return out_buf; }
    if (c == 27) {
        char seq[8] = {0};
        delay_ms(1);
        int i = 0;
        while (i < 7 && read(0, &seq[i], 1) > 0) i++;
        if (i == 0) return "Escape";
        for (int j = 0; j < (int)(sizeof(key_table)/sizeof(KeyMap)); j++) {
            const char *s1 = seq, *s2 = key_table[j].seq;
            while (*s1 && *s1 == *s2) { s1++; s2++; }
            if (*s1 == *s2) return key_table[j].name; }
        return ""; }
    switch (c) {
        case 3:   return "Ctrl+C";
        case 9:   return "Tab";
        case 10:  return "Enter";
        case 32:  return "Space";
        case 127: return "Backspace";
        default: 
            if (c < 32) { out_buf[0] = 'C'; out_buf[1] = 't'; out_buf[2] = 'r'; out_buf[3] = 'l';
                out_buf[4] = '+'; out_buf[5] = c + 64; out_buf[6] = '\0'; } 
            else { out_buf[0] = c; out_buf[1] = '\0'; }
    return out_buf; } }
