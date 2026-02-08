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
#include <mach-o/dyld.h>
#include <sys/ioctl.h>   
#include <ifaddrs.h>    
#include <net/if.h>     
#include <sys/sysctl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <mach/mach_time.h>

#include "sys.h"

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
void  os_memset(void* ptr, int val, size_t size) { unsigned char* p = (unsigned char*)ptr; while (size--) *p++ = (unsigned char)val; }
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
    static uint64_t cpu_hz = 0;
    if (cpu_hz == 0) { struct timespec ts = {0, 10000000L}; uint64_t start = get_cycles();
        nanosleep(&ts, NULL); cpu_hz = (get_cycles() - start) * 100; if (cpu_hz == 0) cpu_hz = 1; }
    uint64_t total_cycles = (uint64_t)ms * (cpu_hz / 1000); uint64_t start_time = get_cycles();
    if (ms > 2) { struct timespec sleep_ts = {0, (ms - 1) * 1000000L}; nanosleep(&sleep_ts, NULL); }
    struct timespec check_start; clock_gettime(CLOCK_MONOTONIC_COARSE, &check_start); uint32_t safety = 0;
    while ((get_cycles() - start_time) < total_cycles) { __asm__ volatile("pause");
        if (++safety > 2000) { struct timespec now; clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
                               if (now.tv_sec > check_start.tv_sec) { cpu_hz = 0; break; }
                               safety = 0; } } }
/*___________________________________________________________________________*/
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
    if (read(0, p, 1) <= 0) return b;
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
        case 10:
        case 13: *p = K_ENT; return b;
        case 32: *p = K_SPA; return b;
        case  8:
        case 127: *p = K_BAC; return b;
        case 27: {
            int i = 0;
            while (i < 4 && read(0, p + i, 1) > 0) i++;
            if (i == 0) { *p = K_ESC; return b; }
            for (int j = 0; j < (int)(sizeof(nameid)/sizeof(KeyIDMap)); j++) {
                const char *s1 = p, *s2 = nameid[j].name;
                while (*s1 && *s1 == *s2) { s1++; s2++; }
                if (*s1 == '\0' && *s2 == '\0') { 
                    *p++ = nameid[j].id; *p = 0; return b; } } }
        default: *p = 0; return b; } }      
/*___________________________________________________________________________*/

uint64_t get_cycles(void) { return mach_absolute_time(); }
    
static unsigned char* GlobalBuf = NULL; 
static size_t GlobalLen = 0;
unsigned char* GetBuff(size_t *size) {
    GlobalLen = (*size + 0xFFF) & ~0xFFF;
    void *ptr = mmap(NULL, GlobalLen, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) { GlobalBuf = NULL; GlobalLen = 0; return NULL; }
    GlobalBuf = (unsigned char*)ptr; *size = GlobalLen; return GlobalBuf; }

void FreeBuff(void) {
    if (GlobalBuf) { munmap(GlobalBuf, GlobalLen); GlobalBuf = NULL; GlobalLen = 0; } }
    
void SWD(void) {
    uint32_t len = 4096;
    char *path = (char *)GlobalBuf;
    if (_NSGetExecutablePath(path, &len) != 0) return;
    for (char *p = path + len; p > path; p--) {
        if (*p == '/') { 
            *p = '\0'; 
            chdir(path); 
            break; } } }
void UniversalHwid(void) {
    unsigned char* curr = GlobalBuf;
    unsigned char* const end = GlobalBuf + 5119;
    char serialNum[256];
    size_t listSize = sizeof(serialNum);
    if (sysctlbyname("hw.serialnumber", serialNum, &listSize, NULL, 0) == 0) {
        for (char *s = serialNum; *s && curr < end; s++) {
            if ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')) {
                *curr++ = *s; } } }
    char osVersion[256];
    listSize = sizeof(osVersion);
    if (sysctlbyname("kern.osrelease", osVersion, &listSize, NULL, 0) == 0) {
       for (char *v = osVersion; *v && curr < end; v++) {
            if ((*v >= '0' && *v <= '9') || (*v >= 'a' && *v <= 'z') || (*v >= 'A' && *v <= 'Z') || *v == '.') {
                *curr++ = *v; } } }
    *curr = '\0';
    int len = (int)(curr - GlobalBuf);
    const char* map = "ABCDEFGHIJKLMNOPQRSTUVWXYZ013579";
    if (len > 32) {
        unsigned char* s = GlobalBuf + 32;
        unsigned char* d = GlobalBuf;
        unsigned char* stop = GlobalBuf + len;
        while (s < stop) {
            *d++ ^= *s++;
            if (d == GlobalBuf + 32) d = GlobalBuf; } } 
    else if (len < 32) {
        unsigned char* d = GlobalBuf + len;
        while (d < GlobalBuf + 32) { *d = *(d - len); d++; } }
    unsigned char* p = GlobalBuf;
    while (p < GlobalBuf+32) { size_t offset = (size_t)(p - GlobalBuf); *p = *(map + ((*p ^ (offset * 7)) & 31)); p++; }
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
int AutoEncryptOrValidate(const char *fname) {
    static int hw_ok = 0; if (!hw_ok) { UniversalHwid(); hw_ok = 1; }
    void* h = os_open_file(fname); if (!h) return 1;
    unsigned char *f_data = GlobalBuf + 512;
    int r = os_read_file(h, f_data, 3583); os_close_file(h);
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
                unsigned char *raw = GlobalBuf + 32; unsigned char *r_ptr = raw;
                unsigned char *src = p_start; while (src < sp) *r_ptr++ = *src++;
                src = p_st; while (src < p_en) *r_ptr++ = *src++;
                for (int i = 0; i < (le + lp); i++) *(raw + i) ^= *(GlobalBuf + (i & 31));
                Crypt(raw, le + lp);
                char *hdr = (char*)(GlobalBuf + 162);
                ValToHex((unsigned char)le, hdr); ValToHex((unsigned char)lp, hdr + 2);
                for (int i = 0; i < (le + lp); i++) ValToHex(*(raw + i), hdr + 4 + i * 2);
                int h_len = 4 + (le + lp) * 2;
                unsigned char *out = GlobalBuf + 4096; unsigned char *o = out;
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
    unsigned char *p = GlobalBuf + 512; 
    int r = os_read_file(h, p, 7);
    if (r >= 3 && *p == 0xEF) p += 3;
    int le = (HexVal(*p) << 4) | HexVal(*(p+1));
    int lp = (HexVal(*(p+2)) << 4) | HexVal(*(p+3));
    if (le + lp > 128 || le < 5 || lp < 8) { os_close_file(h); return 1; }
    unsigned char *hex_in = GlobalBuf + 512; // Переиспользуем зону чтения
    unsigned char *raw    = GlobalBuf + 32;  // HWID кончился на 31
    unsigned char *em     = GlobalBuf + 200; // Почта
    unsigned char *pw     = GlobalBuf + 300; // Пароль
    unsigned char *cmd    = GlobalBuf + 512; // Команда (перетрет hex_in потом)
    os_read_file(h, hex_in, (le + lp) * 2); os_close_file(h);
    for (int i = 0; i < (le + lp); i++)
        *(raw + i) = (unsigned char)((HexVal(*(hex_in + i*2)) << 4) | HexVal(*(hex_in + i*2 + 1)));
    Crypt(raw, le + lp);
    for (int i = 0; i < (le + lp); i++) *(raw + i) ^= *(GlobalBuf + (i & 31));
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
