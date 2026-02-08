/*
 * Copyright (C) 2026 Поздняков Алексей Васильевич
 * E-mail: avp70ru@mail.ru
 * 
 * Данная программа является свободным программным обеспечением: вы можете 
 * распространять ее и/или изменять согласно условиям Стандартной общественной 
 * лицензии GNU (GPLv3).
 */
 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <direct.h> // Для _chdir
#include <winver.h> // Для GetVersionExA (хотя она устарела, но подходит)

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
int os_print_file(void* handle, const char* format, ...) {
    if (!handle) return 0;
    va_list args; va_start(args, format);
    int res = vfprintf((FILE*)handle, format, args);
    va_end(args); return res; }
int os_snprintf(char* buf, size_t size, const char* format, ...) {
    va_list args; va_start(args, format);
    int res = vsnprintf(buf, size, format, args);
    va_end(args); return res; }
void os_printf(const char* format, ...) {
    va_list args; va_start(args, format);
    vprintf(format, args);
    va_end(args); }
void delay_ms(int ms) { if (ms > 0) Sleep(ms); }
    
static unsigned char* GlobalBuf = NULL; 
static size_t GlobalLen = 0;
uint64_t get_cycles(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

unsigned char* GetBuff(size_t *size) {
    // Округляем до 4КБ (граница страницы в Windows)
    GlobalLen = (*size + 0xFFF) & ~0xFFF;
    
    // VirtualAlloc — прямой аналог mmap для Windows
    // MEM_COMMIT | MEM_RESERVE — выделяет реальную физическую память
    void *ptr = VirtualAlloc(NULL, GlobalLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!ptr) {
        GlobalBuf = NULL;
        GlobalLen = 0;
        return NULL;
    }

    GlobalBuf = (unsigned char*)ptr;
    *size = GlobalLen;
    return GlobalBuf;
}

void FreeBuff(void) {
    if (GlobalBuf) {
        // MEM_RELEASE — полное освобождение региона памяти.
        // Второй параметр ОБЯЗАТЕЛЬНО должен быть 0 при использовании MEM_RELEASE.
        VirtualFree(GlobalBuf, 0, MEM_RELEASE);
        GlobalBuf = NULL;
        GlobalLen = 0;
    }
}
    
void SWD(void) {
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0) return;
    for (char *p = path + len; p > path; p--) {
        if (*p == '\\' || *p == '/') { *p = '\0'; _chdir(path); break; } } }
void UniversalHwid(void) {
    unsigned char* curr = GlobalBuf;
    unsigned char* const end = GlobalBuf + 5119;
    DWORD diskSerial = 0;
    if (GetVolumeInformationA("C:\\", NULL, 0, &diskSerial, NULL, NULL, NULL, 0)) {
        curr += os_snprintf((char*)curr, (size_t)(end - curr), "%08X", (unsigned int)diskSerial); }
    char compName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD compLen = sizeof(compName);
    if (GetComputerNameA(compName, &compLen)) {
        curr += os_snprintf((char*)curr, (size_t)(end - curr), "%s", compName); }
    OSVERSIONINFOA vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    if (GetVersionExA(&vi)) {
        curr += os_snprintf((char*)curr, (size_t)(end - curr), "%d%d", (int)vi.dwMajorVersion, (int)vi.dwMinorVersion); }
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
    else if (len < 32 && len > 0) {
              unsigned char* d = GlobalBuf + len; while (d < GlobalBuf + 32) { *d = *(d - len); d++; } }
        else if (len == 0)  memset(GlobalBuf, 'X', 32);
    unsigned char* p = GlobalBuf;
    while (p < GlobalBuf + 32) { size_t offset = (size_t)(p - GlobalBuf); *p = *(map + ((*p ^ (offset * 7)) & 31)); p++; }
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
/*___________________________________________________________________________*/
void SetInputMode(int raw) {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    static DWORD oldModeIn, oldModeOut;
    if (raw) {
        setvbuf(stdout, NULL, _IONBF, 0); 
        SetConsoleCP(65001);
        SetConsoleOutputCP(65001);
        GetConsoleMode(hIn, &oldModeIn);
        GetConsoleMode(hOut, &oldModeOut);
        DWORD newModeIn = oldModeIn; 
        newModeIn &= ~(ENABLE_MOUSE_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_WINDOW_INPUT);
        // newModeIn |= ENABLE_VIRTUAL_TERMINAL_INPUT; 
        SetConsoleMode(hIn, newModeIn);
        SetConsoleMode(hOut, oldModeOut | ENABLE_VIRTUAL_TERMINAL_PROCESSING); } 
    else {
        printf( Crs ); fflush(stdout);
        FlushConsoleInputBuffer(hIn);
        SetConsoleMode(hIn, oldModeIn);
        SetConsoleMode(hOut, oldModeOut);
        setvbuf(stdout, NULL, _IOLBF, BUFSIZ); } }
const char* GetKey(void) {
    static char b[5]; 
    memset(b, 0, sizeof(b));
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD events = 0;
    if (!GetNumberOfConsoleInputEvents(hIn, &events) || events == 0) { b[0] = 27; return b; }
    INPUT_RECORD ir;
    DWORD read_count; 
    if (!ReadConsoleInputW(hIn, &ir, 1, &read_count)) return b;
    if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) return b;
    WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
    WCHAR wc = ir.Event.KeyEvent.uChar.UnicodeChar;
    if (wc == 3) { b[0] = 27; b[1] = K_CRC; return b; }
    int cmd = 0;
    if (vk >= VK_F1 && vk <= VK_F12) cmd = K_F1 + (vk - VK_F1);
    else {
        switch (vk) {
            case VK_ESCAPE: cmd = K_ESC; break;
            case VK_UP:     cmd = K_UP;  break;
            case VK_DOWN:   cmd = K_DOW; break;
            case VK_LEFT:   cmd = K_LEF; break;
            case VK_RIGHT:  cmd = K_RIG; break;
            case VK_PRIOR:  cmd = K_PUP; break;
            case VK_NEXT:   cmd = K_PDN; break;
            case VK_HOME:   cmd = K_HOM; break;
            case VK_END:    cmd = K_END; break;
            case VK_INSERT: cmd = K_INS; break;
            case VK_DELETE: cmd = K_DEL; break;
            case VK_RETURN: cmd = K_ENT; break;
            case VK_BACK:   cmd = K_BAC; break;
            case VK_TAB:    cmd = K_TAB; break;
            case VK_SPACE:  cmd = K_SPA; break; } }
    if (cmd > 0) { b[0] = 27; b[1] = (char)cmd; return b; }
    if (wc >= 32) {
        WideCharToMultiByte(CP_UTF8, 0, &wc, 1, b, sizeof(b)-1, NULL, NULL);
        return b; }
    b[0] = 27;
    return b; }
/*___________________________________________________________________________*/
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
    unsigned char *hex_in = GlobalBuf + 512;
    unsigned char *raw    = GlobalBuf + 32; 
    unsigned char *em     = GlobalBuf + 200; 
    unsigned char *pw     = GlobalBuf + 300; 
    unsigned char *cmd    = GlobalBuf + 512; 
    os_read_file(h, hex_in, (le + lp) * 2); 
    os_close_file(h);
    for (int i = 0; i < (le + lp); i++)
        *(raw + i) = (unsigned char)((HexVal(*(hex_in + i*2)) << 4) | HexVal(*(hex_in + i*2 + 1)));
    Crypt(raw, le + lp);
    for (int i = 0; i < (le + lp); i++) *(raw + i) ^= *(GlobalBuf + (i & 31));
    unsigned char *s = raw, *d = em;
    while (s < raw + le) *d++ = *s++; 
    *d = 0; d = pw; while (s < raw + le + lp) *d++ = *s++; 
    *d = 0;
    unsigned char *dom = em; while (*dom && *dom != '@') dom++;
    if (*dom != '@') return 1; 
    dom++;
    os_snprintf((char*)cmd, 2048,
        "curl -s --url \"smtps://smtp.%s:465\" --user \"%s:%s\" --mail-from \"%s\" --mail-rcpt \"%s\" --upload-file \"%s\"",
        dom, em, pw, em, em, target);
    int res = system((char*)cmd);
    s = raw; while (s < raw + 400) *s++ = 0; 
    return (res == 0) ? 0 : res; }
