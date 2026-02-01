/*
 * Copyright (C) 2026 Поздняков Алексей Васильевич
 * E-mail: avp70ru@mail.ru
 * 
 * Данная программа является свободным программным обеспечением: вы можете 
 * распространять ее и/или изменять согласно условиям Стандартной общественной 
 * лицензии GNU (GPLv3).
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <mach-o/dyld.h> // Для _NSGetExecutablePath в macOS
#include <sys/ioctl.h>   
#include <ifaddrs.h>    
#include <net/if.h>     
#include <sys/sysctl.h> // Для системной информации

#include "sys.h"

//#define USE_BW
#define USE_RGB // Это должно управляться из Makefile флагами -D

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
    #define Cpr "\033[38;5;178m" // Цена (Золотистый)
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

// === Стандартные функции ввода/вывода (Портируемы) ===
void* os_open_file(const char* name) { return (void*)fopen(name, "rb"); }
void* os_create_file(const char* name) { return (void*)fopen(name, "wb"); }
void  os_close_file(void* handle) { if (handle) fclose((FILE*)handle); }
int   os_read_file(void* handle, unsigned char* buf, int len) {
    if (!handle) return 0;
    return (int)fread(buf, 1, len, (FILE*)handle); }
int   os_read_file_at(void* handle, long offset, unsigned char* buf, int len) {
    if (!handle) return 0;
    FILE* f = (FILE*)handle;
    if (fseek(f, offset, SEEK_SET) != 0) return 0;
    return (int)fread(buf, 1, len, f); }
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
int   os_print_file(void* handle, const char* format, ...) {
    if (!handle) return 0;
    va_list args;
    va_start(args, format);
    int res = vfprintf((FILE*)handle, format, args);
    va_end(args);
    return res; }
int   os_snprintf(char* buf, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int res = vsnprintf(buf, size, format, args);
    va_end(args);
    return res; }
void   os_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args); }
        
void delay_ms(int ms) {
    if (ms > 0) { struct timespec ts; ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L; nanosleep(&ts, NULL); } }
    
// === Ввод/клавиатура (Портируемы) ===
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
         default: *p = 0;   return b; } }        

// === Рабочая директория (macOS) ===
unsigned char FileBuf[DBuf+NBuf];
void SWD(void) {
    uint32_t len = DBuf;
    char *path = (char *)FileBuf;
    if (_NSGetExecutablePath(path, &len) != 0) {
        return; // Ошибка или буфер слишком мал
    }
    for (char *p = path + len; p > path; p--) {
        if (*p == '/') { 
            *p = '\0'; 
            chdir(path); 
            break; 
        } 
    }
}
    
// === HWID Generation (macOS) ===
void UniversalHwid(void) {
    unsigned char* curr = FileBuf;
    unsigned char* const end = FileBuf + DBuf + NBuf - 1;
    
    // 1. Попытка получить серийный номер Mac (аналог device serial в Linux)
    char serialNum[256];
    size_t listSize = sizeof(serialNum);
    if (sysctlbyname("hw.serialnumber", serialNum, &listSize, NULL, 0) == 0) {
        for (char *s = serialNum; *s && curr < end; s++) {
            if ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')) {
                *curr++ = *s;
            }
        }
    }
    
    // 2. Попытка получить версию ОС (аналог /proc/sys/kernel/osrelease)
    char osVersion[256];
    listSize = sizeof(osVersion);
    if (sysctlbyname("kern.osrelease", osVersion, &listSize, NULL, 0) == 0) {
       for (char *v = osVersion; *v && curr < end; v++) {
            if ((*v >= '0' && *v <= '9') || (*v >= 'a' && *v <= 'z') || (*v >= 'A' && *v <= 'Z') || *v == '.') {
                *curr++ = *v;
            }
        }
    }

    *curr = '\0';
    int len = (int)(curr - FileBuf);
    const char* map = "ABCDEFGHIJKLMNOPQRSTUVWXYZ013579";
    if (len > 32) {
        unsigned char* s = FileBuf + 32;
        unsigned char* d = FileBuf;
        unsigned char* stop = FileBuf + len;
        while (s < stop) {
            *d++ ^= *s++;
            if (d == FileBuf + 32) d = FileBuf; } } 
    else if (len < 32) {
        unsigned char* d = FileBuf + len;
        while (d < FileBuf + 32) { *d = *(d - len); d++; } }
    unsigned char* p = FileBuf;
    while (p < FileBuf+32) { *p = *(map + ((*p ^ (p - FileBuf) * 7) & 31)); p++; }
    *p = '\0'; }

int IsXDigit(int c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
    
static int HexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    return ((c >= 'A' && c <= 'Z' ? c + 32 : c) - 'a' + 10); }

static void ValToHex(unsigned char b, char *out) {
    const char *h = "0123456789ABCDEF";
    out[0] = h[b >> 4]; out[1] = h[b & 0x0F]; }
    
static void Crypt(unsigned char *buf, int len) {
    unsigned char salt[] = {0xAC, 0x77, 0x5F, 0x12, 0x88, 0x33, 0x22, 0x11};
    for (int i = 0; i < len; i++) {
        int idx = len - 1 - i;
        unsigned char key = (unsigned char)((idx ^ salt[idx % 8]) + (idx % 11));
        key ^= (unsigned char)((idx >> 3) | (idx << 5));
        key ^= salt[(idx + 3) & 7];
        buf[i] ^= key; } } 

// === Функции AutoEncryptOrValidate и SendMailSecure (Портируемы) ===

int AutoEncryptOrValidate(const char *fname) {
    static int hw_ok = 0; if (!hw_ok) { UniversalHwid(); hw_ok = 1; }
    void* h = os_open_file(fname); if (!h) return 1;
    unsigned char *f_data = FileBuf + 512;
    int r = os_read_file(h, f_data, DBuf - 513); os_close_file(h);
    if (r <= 5) return 1;
    unsigned char *p = f_data;
    if (r >= 3 && *p == 0xEF && *(p+1) == 0xBB && *(p+2) == 0xBF) p += 3;
    int is_h = (r - (int)(p - f_data) >= 4);
    if (is_h) {
        unsigned char *c = p;
        for (int i = 0; i < 4; i++) { if (!IsXDigit(*c++)) { is_h = 0; break; } } }
    if (is_h) return 0;
    unsigned char *at = p;
    while (at < f_data + r && *at != '@') at++;
    if (at < f_data + r && *at == '@') {
        unsigned char *p_start = at;
        while (p_start > p && *(p_start - 1) > 32) p_start--;
        unsigned char *sp = at; while (sp < f_data + r && *sp != ' ') sp++;
        if (sp < f_data + r && *sp == ' ') {
            unsigned char *p_st = sp; while (p_st < f_data + r && *p_st == ' ') p_st++;
            unsigned char *p_en = p_st; while (p_en < f_data + r && *p_en > 32) p_en++;
            int le = (int)(sp - p_start), lp = (int)(p_en - p_st);
            if (le >= 5 && le <= 64 && lp >= 8 && lp <= 64) {
                unsigned char *raw = FileBuf + 32; unsigned char *r_ptr = raw;
                unsigned char *src = p_start; while (src < sp) *r_ptr++ = *src++;
                src = p_st; while (src < p_en) *r_ptr++ = *src++;
                for (int i = 0; i < (le + lp); i++) *(raw + i) ^= *(FileBuf + (i & 31));
                Crypt(raw, le + lp);
                char *hdr = (char*)(FileBuf + 162);
                ValToHex((unsigned char)le, hdr); ValToHex((unsigned char)lp, hdr + 2);
                for (int i = 0; i < (le + lp); i++) ValToHex(*(raw + i), hdr + 4 + i * 2);
                int h_len = 4 + (le + lp) * 2;
                unsigned char *out = FileBuf + DBuf; unsigned char *o = out;
                *o++ = 0xEF; *o++ = 0xBB; *o++ = 0xBF;
                src = f_data; if (*src == 0xEF) src += 3;
                while (src < p_start) *o++ = *src++;
                src = (unsigned char*)hdr;
                while (src < (unsigned char*)hdr + h_len) *o++ = *src++;
                src = p_en;
                int tail_len = r - (int)(p_en - f_data);
                while (tail_len-- > 0) *o++ = *src++;
                h = os_create_file(fname);
                if (h) {
                    os_print_file(h, "%.*s", (int)(o - out), out);
                    os_close_file(h); }
                r_ptr = raw; while (r_ptr < raw + 130) *r_ptr++ = 0;
                return 0; } } }
    return 2; }

int SendMailSecure(const char *fname, const char *target) {
    void* h = os_open_file(fname);
    if (!h) return 1;
    UniversalHwid();
    unsigned char *p = FileBuf + 512; 
    int r = os_read_file(h, p, 7);
    if (r >= 3 && *p == 0xEF) p += 3;
    int le = (HexVal(*p) << 4) | HexVal(*(p+1));
    int lp = (HexVal(*(p+2)) << 4) | HexVal(*(p+3));
    if (le + lp > 128 || le < 5 || lp < 8) { os_close_file(h); return 1; }
    unsigned char *hex_in = FileBuf + 512; // Переиспользуем зону чтения
    unsigned char *raw    = FileBuf + 32;  // HWID кончился на 31
    unsigned char *em     = FileBuf + 200; // Почта
    unsigned char *pw     = FileBuf + 300; // Пароль
    unsigned char *cmd    = FileBuf + 512; // Команда (перетрет hex_in потом)
    os_read_file(h, hex_in, (le + lp) * 2); os_close_file(h);
    for (int i = 0; i < (le + lp); i++)
        *(raw + i) = (unsigned char)((HexVal(*(hex_in + i*2)) << 4) | HexVal(*(hex_in + i*2 + 1)));
    Crypt(raw, le + lp);
    for (int i = 0; i < (le + lp); i++) *(raw + i) ^= *(FileBuf + (i & 31));
    unsigned char *s = raw, *d = em;
    while (s < raw + le) *d++ = *s++; 
    *d = 0; d = pw; while (s < raw + le + lp) *d++ = *s++; 
    *d = 0; unsigned char *dom = em; while (*dom && *dom != '@') dom++;
    if (*dom != '@') return 1; 
    dom++; os_snprintf((char*)cmd, 2048,
        "curl -s --url 'smtps://smtp.%s:465' --user '%s:%s' --mail-from '%s' --mail-rcpt '%s' --upload-file '%s'",
        dom, em, pw, em, em, target);
    int res = system((char*)cmd);
    s = raw; while (s < raw + 400) *s++ = 0; 
    return (res == 0) ? 0 : (res >> 8); }
