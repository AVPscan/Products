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

void delay_ms(int ms) {
    if (ms > 0) { struct timespec ts; ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L; nanosleep(&ts, NULL); } }
    
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

typedef struct { const char *name; unsigned char id; } KeyIDMap;
KeyIDMap nameid[] = {
    {"[A", K_UP}, {"[B", K_DOW}, {"[C", K_RIG}, {"[D", K_LEF},
    {"[H", K_HOM}, {"[F", K_END}, {"[1~", K_HOM}, {"[4~", K_END},
    {"[2~", K_INS}, {"[3~", K_DEL}, {"[5~", K_PUP}, {"[6~", K_PDN},
    {"OP", K_F1}, {"OQ", K_F2}, {"OR", K_F3}, {"OS", K_F4},
    {"[15~", K_F5}, {"[17~", K_F6}, {"[18~", K_F7}, {"[19~", K_F8},
    {"[20~", K_F9}, {"[21~", K_F10}, {"[23~", K_F11}, {"[24~", K_F12} };
const char* GetKey(void) {
    static char b[6]; char *p = b;
    os_memset(b, 0, sizeof(b));
    if (read(0, p, 1) <= 0) { *p =27; return b; }
    unsigned char c = *(unsigned char*)p;
    if (c > 127) {
        int len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
        while (--len > 0) read(0, ++p, 1);
        return b; }
    if (c > 32 && c < 127 ) return b;
    *p++ = 27;
    switch (c) {
         case  3: *p = K_CRC; return b;
         case  9: *p = K_TAB; return b;
         case 10: *p = K_ENT; return b;
         case 32: *p = K_SPA; return b;
        case 127: *p = K_BAC; return b;
         case 27: {
                  int i = 0;
                  while (i < 4 && read(0, p + i, 1) > 0) i++;
                  if (i == 0) { b[1] = K_ESC; return b; }
                  for (int j = 0; j < (int)(sizeof(nameid)/sizeof(KeyIDMap)); j++) {
                      const char *s1 = p, *s2 = nameid[j].name;
                      while (*s1 && *s1 == *s2) { s1++; s2++; }
                      if (*s1 == '\0' && *s2 == '\0') { 
                          *p++ = nameid[j].id; *p = 0; return b; } } }
                  __attribute__((fallthrough));
         default: *p = 0;   return b; } }        

