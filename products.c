/*
 * Copyright (C) 2026 Поздняков Алексей Васильевич
 * E-mail: avp70ru@mail.ru
 * 
 * Данная программа является свободным программным обеспечением: вы можете 
 * распространять ее и/или изменять согласно условиям Стандартной общественной 
 * лицензии GNU (GPLv3).
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sys.h"

#define DBase "products.txt"
#define DRep  "reports.txt"
#define DAn   "analitics.txt"
#define DSend "send.txt"
#define Mname 32
#define BufN Mname*4

char nc[BufN+1];
extern unsigned char FileBuf[DBuf+NBuf];
static int LH[Mname];
static int kpr = 0;

typedef struct { char* name; int price, qy, summa, tqy, vis, nameC,FCN; } DicDat;
typedef struct { 
    DicDat* dat; 
    int cap, count, MaxP, MaxQ, MaxS, MaxT, MaxV,FMN,FMP,FMQ,FMS,FMT,FMV;
    int Fsum[3]; // 0:pro, 1:res, 2:ana
} Dic;

int StringBC(const char *s, int *c) { 
    int b = 0, i = 0; if (!s) { if (c) *c = 0; return 0; } 
    while (s[b]) { if ((s[b] & 0xC0) != 0x80) i++; b++; } 
    if (c) *c = i;
    return b; }
    
int StrLenB(const char *s) {
    return s ? strlen(s) : 0; }
    
int StrLen(const char *s) {
    if (!s) return 0; 
    const unsigned char *p = (const unsigned char *)s;
    int count = 0; 
    while (*p) { if ((*p++ & 0xC0) != 0x80) count++; }
    return count; }
       
int CharType(const unsigned char* buf, int* len) {
    unsigned char c = *buf; int l;
    if (c < 128) { *len = 1;
        if (c >= '0' && c <= '9') return 1;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return 2;
        if (c >= 33 && c <= 126) return 4;
        if (c == 0) return 5;
        return 3; }
    if ((c & 0xE0) == 0xC0)      l = 2;
    else if ((c & 0xF0) == 0xE0) l = 3;
        else if ((c & 0xF8) == 0xF0) l = 4;
            else { *len = 1; return 5; }
    *len = l; for (int i = 1; i < l; i++) if ((buf[i] & 0xC0) != 0x80) return 5;
    return 2; }

char* STU(const char* s) {
    int i = 0;
    while (s[i] && i < (BufN - 1)) {
        if (s[i] == ' ') nc[i] = '_';
        else nc[i] = s[i];
        i++; }
    nc[i] = '\0'; return nc; }
    
void ClearDic(Dic* Pro) {
    if (!Pro) return;
    if (Pro->dat) { for (int i = 0; i < Pro->count; i++) os_free(Pro->dat[i].name);
                    os_free(Pro->dat); }
    os_memset(Pro, 0, sizeof(Dic)); }

typedef struct { int n3, n2, n1, bnam, cnam, sum, tqy, qy, tst; } PS_t;
PS_t PS;
int AddDicFull(Dic* Pro, const char* name, int summa, int tqy, int qy, int mode) {
    int low = 0, cmp = 0, high = Pro->count - 1, mid, r, s;
    if (Pro->FMN < PS.cnam) Pro->FMN = PS.cnam;
    while (low <= high) {
        mid = (low + high) / 2;
        if ((cmp = strcmp(name, Pro->dat[mid].name)) == 0) {
            if (mode) {
                Pro->dat[mid].summa += summa; 
                Pro->dat[mid].tqy += tqy; 
                Pro->dat[mid].vis += qy;
                for (r = 1, s = Pro->dat[mid].summa; s > 9; s /= 10, r++);
                if (Pro->MaxS < Pro->dat[mid].summa) { Pro->MaxS = Pro->dat[mid].summa; Pro->FMS = r; }
                for (r = 1, s = Pro->dat[mid].tqy; s > 9 ; s /= 10, r++);
                if (Pro->MaxT < Pro->dat[mid].tqy) { Pro->MaxT = Pro->dat[mid].tqy; Pro->FMT = r; }
                for (r = 1, s = Pro->dat[mid].vis; s > 9 ; s /= 10, r++);
                if (Pro->MaxV < Pro->dat[mid].vis) { Pro->MaxV = Pro->dat[mid].vis; Pro->FMV = r; } }
            else { Pro->dat[mid].price = summa;
                if (tqy) Pro->dat[mid].qy += tqy;
                else Pro->dat[mid].qy = 0;
                if (Pro->dat[mid].qy > 99) Pro->dat[mid].qy=99;
                PS.tqy = 1; if (Pro->dat[mid].qy > 9) PS.tqy++;
                if (Pro->MaxP < summa) { Pro->MaxP = summa; Pro->FMP = PS.sum; }
                if (Pro->MaxQ < tqy) { Pro->MaxQ = tqy; Pro->FMQ = PS.tqy; } }
            return mid; }
        if (cmp < 0) high = mid - 1;
        else low = mid + 1; }
    if (Pro->count >= Pro->cap) {
        Pro->cap = (Pro->cap == 0) ? 2 : Pro->cap * 2;
        DicDat* tmp = os_realloc(Pro->dat, Pro->cap * sizeof(DicDat));
        if (!tmp) return -1;
        Pro->dat = tmp; }
    for (int i = Pro->count; i > low; i--) Pro->dat[i] = Pro->dat[i-1];
    Pro->dat[low].name = os_strdup(name); Pro->dat[low].nameC = PS.cnam; Pro->dat[low].FCN = PS.bnam - PS.cnam;
    if (mode) { Pro->dat[low].summa = summa; Pro->dat[low].tqy = tqy; Pro->dat[low].vis = qy; Pro->dat[low].qy = 0;
                Pro->dat[low].price = summa/tqy; for (r = 1, s = Pro->dat[low].price; s > 9 ; s /= 10, r++);
                if (Pro->MaxS < summa) { Pro->MaxS = summa; Pro->FMS = PS.sum; }
                if (Pro->MaxT < tqy) { Pro->MaxT = tqy; Pro->FMT = PS.tqy; }
                if (Pro->MaxV < qy) { Pro->MaxV = qy; Pro->FMV = PS.qy; } 
                PS.sum = r; tqy = 0; PS.tqy = 1; }
    else {  if (tqy > 99) { tqy = 99; PS.tqy = 2; }
            Pro->dat[low].price = summa; Pro->dat[low].qy = tqy;
            Pro->dat[low].summa = 0; Pro->dat[low].tqy = 0; Pro->dat[low].vis = 0; 
            if (Pro->MaxS <= 0) { Pro->MaxS = 0; Pro->FMS = 1; }
            if (Pro->MaxT <= 0) { Pro->MaxT = 0; Pro->FMT = 1; }
            if (Pro->MaxV <= 0) { Pro->MaxV = 0; Pro->FMV = 1; } }
    if (Pro->MaxQ < tqy) { Pro->MaxQ = tqy; Pro->FMQ = PS.tqy; }
    if (Pro->MaxP < Pro->dat[low].price) { Pro->MaxP = Pro->dat[low].price; Pro->FMP = PS.sum; } 
    Pro->count++; return low; }
int AddDic(Dic* Pro, const char* name, int summa, int tqy) { int i;
    if ( !name || name[0] == 0 || summa < 1 || tqy < 0) return -1;
    PS.sum = 1; for ( i = summa; i > 9 ; i /= 10,  PS.sum++);
    PS.tqy = 1; for ( i = tqy; i > 9 ; i /= 10,  PS.tqy++);
    PS.bnam= StringBC(name, &PS.cnam);
    return AddDicFull(Pro, name, summa, tqy, 0, 0); }
void ParseBuf( Dic* Pro, unsigned char* buf, unsigned char* out, int mode) {
    int type,len,i,j; 
      while (buf < out) {
        if ((type = CharType(buf, &len)) == 1) { while (buf < out && (type = CharType(buf, &len)) == 1 && *buf == '0') buf++ ;
                       if (type == 1) { PS.n3 = PS.n2; PS.n2 = PS.n1; PS.n1 = 0 ;
                          PS.qy = PS.tqy; PS.tqy = PS.sum; PS.sum = 0;
                          while (buf < out && (type = CharType(buf, &len)) == 1 && PS.sum < 9 )
                                { PS.n1 = PS.n1 * 10 + *buf - '0'; PS.sum++ ; buf++ ; } 
                          while (buf < out && (type = CharType(buf, &len)) == 1) buf++ ; } }
        if (type == 2) { PS.cnam = 0; j = 0; int sq = 0; 
            while (buf < out) { type = CharType(buf, &len);
                if (type == 1 || type == 3) break; 
                if (type == 2) { for (i = 0; i < len && j < BufN; i++) nc[j++] = buf[i];
                    PS.cnam++; sq = 0; } 
                else if (type == 4) { if (j > 0 && !sq) { nc[j++] = ' '; sq = 1; PS.cnam++; } }
                buf += len; }
            if (j > 0 && nc[j-1] == ' ') { j--; PS.cnam--; }
            nc[j] = '\0'; PS.bnam = j;
            buf -= len; }
        if (PS.cnam) switch (mode) {
                        case 0: if (PS.sum) { j = AddDicFull(Pro, nc, PS.n1, 0, 0, 0); PS.tqy = 0; PS.qy = 0; }
                                __attribute__((fallthrough));
                        case 1: if (PS.tqy) { PS.n1 *= PS.n2; PS.n3 = 1; PS.qy = 1; PS.sum = 1;
                                for ( i = PS.n1; i > 9 ; i /= 10,  PS.sum++); }
                                __attribute__((fallthrough));
                        case 2: if (PS.qy) j = AddDicFull(Pro, nc, PS.n1, PS.n2, PS.n3, 1);
                                PS.tst += PS.n1; PS.cnam = 0; PS.sum = 0; PS.tqy = 0;  PS.qy = 0; }
        buf+=len; } }

int LoadDic(Dic* Pro, const char* filename) {
    int f = -1,i = (strcmp(filename, DBase) == 0) ? 0 : 
                   (strncmp(filename, "rep", 3) == 0) ? 1 : (strncmp(filename, "ana", 3) == 0) ? 2 : 3;
    if (i < 3) {
        f++; size_t Tail = 0; void* File = os_open_file(filename);
        if (File) {
            f++; PS.cnam = 0; PS.sum = 0; PS.tqy = 0;  PS.qy = 0; PS.tst = 0;
            while (1) {
                size_t lf = os_read_file(File, FileBuf + Tail, DBuf); size_t sf = lf + Tail; if (sf == 0) break;
                size_t sl = sf; if (lf == DBuf) { while (sl > 0 && (FileBuf[sl-1] > 32)) sl--;
                                                  if (sl == 0) sl = sf; }
                ParseBuf(Pro, FileBuf, FileBuf+sl, i); Tail = sf - sl; if (Tail > 0) memmove(FileBuf, FileBuf + sl, Tail);
                if (lf < DBuf) break; }
            os_close_file(File); Pro->Fsum[i] = PS.tst; 
            if (PS.n1 > 0 && PS.tst != PS.n1) { Pro->Fsum[i] = 0; f = -2; } } }
    return f; }
    
int SaveDic(Dic* Pro, const char* filename) {
    int k,s,n, i = (strcmp(filename, DBase) == 0) ? 0 : 
                      (strncmp(filename, "rep", 3) == 0) ? 1 : 
                      (strncmp(filename, "ana", 3) == 0) ? 2 : 3;
    if (i == 3) return 0;
    void* f = os_create_file(filename); 
    for (k = 0, s = 0, n = 0; k < Pro->count ; k++) {
        if (i ==0) { s += Pro->dat[k].price; os_print_file(f, "%*d %s\n", Pro->FMP, Pro->dat[k].price, STU(Pro->dat[k].name)); }
        else if (i == 1 && Pro->dat[k].qy > 0) { s += Pro->dat[k].price * Pro->dat[k].qy;
                    os_print_file(f, "%02d %2d %*d %s\n", ++n, Pro->dat[k].qy, Pro->FMP, Pro->dat[k].price, STU(Pro->dat[k].name)); }
             else if (i == 2 && Pro->dat[k].summa > 0) { s += Pro->dat[k].summa;
                          os_print_file(f, "%*d %*d %*d %s\n", Pro->FMV, Pro->dat[k].vis, Pro->FMT, Pro->dat[k].tqy, Pro->FMS,
                                            Pro->dat[k].summa, STU(Pro->dat[k].name)); } }
    Pro->Fsum[i] = s; os_print_file(f, "%d\n", s); os_close_file(f); return Pro->Fsum[i]; }

int PrintDic(Dic* Pro) {
    int n = 0;
    for (int i = 0; i < Pro->count; i++) { 
        if (Pro->dat[i].qy < 1) continue;
        printf(Cnn "%02d ", ++n);
        if (Pro->dat[i].qy > 1) printf(Cnu "%2d ", Pro->dat[i].qy);
        else printf("   ");
        printf(Cnu "%*d " Cna "%-*s\n", 
               Pro->FMP, Pro->dat[i].price, 
               Pro->FMN + Pro->dat[i].FCN, Pro->dat[i].name); }
    return n; }

void Analitics(Dic* Pro) {
    if (!Pro || Pro->count <= 0) return;
    int i, j, k, avg, vp, vv, rp = 0, total_S = 0, total_Q = 0, today_S = 0;
    for (i = 0; i < Pro->count; i++) {
        total_S += Pro->dat[i].summa; total_Q += Pro->dat[i].tqy;
        today_S += Pro->dat[i].price * Pro->dat[i].tqy; }
    if (!total_S || !total_Q) return;
    int *idx = (int*)os_malloc(Pro->count * sizeof(int));
    for (i = 0; i < Pro->count; i++) idx[i] = i;
    for (i = 0; i < Pro->count - 1; i++)
        for (j = i + 1; j < Pro->count; j++)
            if (Pro->dat[idx[i]].summa < Pro->dat[idx[j]].summa) { int t = idx[i]; idx[i] = idx[j]; idx[j] = t; }
    printf(Cls Cna "   %-*s " Cnu "%*s " Cnu "%*s\n", Pro->FMN + 2, "📋", Pro->FMP, "📊", Pro->FMT, " ⚖️ ");
    for (i = 0; i < Pro->count; i++) {
        k = idx[i]; if (Pro->dat[k].tqy == 0 || Pro->MaxV == 0 || Pro->dat[k].summa == 0) continue;
        avg = Pro->dat[k].summa / Pro->dat[k].tqy;
        vp = (Pro->dat[k].tqy * 100) / Pro->MaxV; vv = (Pro->dat[k].price * 100) / avg;
        if (vp > 74) rp += (vp / 100) * Pro->dat[k].price;
        const char* trend = (Pro->dat[k].price > avg) ? Cam "💸 " : (Pro->dat[k].price < avg) ? Cap "🤝 " : Cna "  ";
        const char* vs = (vv < 96) ? Cap : (vv > 104) ? Cam : Cnu;
        const char* vi = (vp > 74) ? Cap : (vp < 33) ? Cam : Cnu;     
        printf(Cnn "%02d " Cna "%-*s %s%*d %s%3d%% %s\n",
               i + 1, Pro->FMN + Pro->dat[k].FCN, Pro->dat[k].name, vs, Pro->FMP, avg, vi, vp, trend); }
    os_free(idx); vv = ((today_S - total_S) * 100) / total_S;
    printf(Cna "\n🏦 🛒 %d 🔮 %d" Cnu "\n        💳 %d ", Pro->MaxV, rp, total_S); fflush(stdout);
    if (vv != 0) printf("%s %d" Cnu " (%s%+d %+d%%" Cnu ")\n", (vv > 0) ? Cam "📈" : Cap "📉", today_S, (vv > 0) ? Cam : Cap, today_S - total_S, vv);
    while (1) {
        delay_ms(60);
        const char* k = GetKey();
        if (k[0] == 27 && k[1] == K_DOW) break; } }

int Fpi(Dic* Pro, const char *s, int *i) {
    if (!Pro || Pro->count <= 0) { if (i) *i = -1; return 0; }
    if (!s || *s == '\0') { if (i) *i = 0; return Pro->count; }
    int in_len = StrLenB(s);
    int low = 0, high = Pro->count - 1;
    int first = -1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strncmp(Pro->dat[mid].name, s, in_len);
        if (cmp == 0) { first = mid; high = mid - 1; }
        else if (cmp < 0) low = mid + 1;
            else high = mid - 1; }
    if (first == -1) { if (i) *i = -1; return 0; }
    if (i) *i = first;
    low = first; 
    high = Pro->count - 1;
    int last = first;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strncmp(Pro->dat[mid].name, s, in_len);
        if (cmp == 0) { last = mid; low = mid + 1; }
        else if (cmp < 0) low = mid + 1;
            else high = mid - 1; }
    return (last - first + 1); }
        
char* prw(Dic* Pro, const char *str1, int i) {
    char *res = (char*)(FileBuf + NBuf);
    int b1, c1, b2, c2, sp;
    b1 = StringBC(str1, &c1);
    if (i < 0 || i >= Pro->count) { sp = Pro->FMN + b1 - c1; sprintf(res, Cna "%-*s", sp, str1); }
    else { const char *full_name = Pro->dat[i].name;
           b2 = StringBC(full_name, &c2); sp = Pro->FMN + b2 - c2; sprintf(res, Cna "%s" Cnn "%-*s", str1, sp - b1, full_name + b1); }
    return res; }
    
typedef struct { char name[BufN+1]; int len,price,lp,col; } IN_t;
IN_t IN;
void Products(Dic* Pro) { 
    int i,tmp,type,b = 0,cr = 0,Pleft = 0,Pnum = 0,num = 0;
    IN.col = -1;
    SetInputMode(1); printf(HCur); tmp = LoadDic(Pro, DBase); tmp = LoadDic(Pro, DAn); tmp = LoadDic(Pro, DRep);
    int flag = !AutoEncryptOrValidate(DSend);
    while (1) {
        if (IN.col == -1) { 
            Pleft = 0; Pnum = 0; IN.col = 1; IN.lp = 0; IN.len = 0; IN.price = 0; IN.name[0] = '\0'; num = -1; kpr = 0;
            if (!Pro || Pro->count < 1) { i = -1; b = 0; }
            else { i = 0; b = Pro->count; }
            cr=i; }
        if (num == -1) {
            printf( Cls Cna ); num = 1 + PrintDic(Pro); printf(SCur); }
        if (cr!=-1) { if (IN.lp == 0) IN.price=Pro->dat[cr].price; }
        printf(LCur Cnn "%02d " Cnu "%2d " Cnu "%*d " Cna "%s ", num, IN.col, Pro->FMP, IN.price, prw(Pro, IN.name, cr)); fflush(stdout);
        delay_ms(120);
        const char *key = GetKey();
        if (key[0] == 27 && key[1] == 0) { 
            if (b > 0) {
                cr = (cr == -1) ? b : (cr - i);
                cr = (cr + 1) % (b + 1);
                if (cr == b) cr = -1;
                else cr += i; }
            else cr = -1;
        continue; }
        if (key[0] != 27) { 
                if ((type=CharType((unsigned char*)key,&tmp))==1) {
                    if (Pleft ) {
                        IN.col=key[0]-'0'; Pleft=0; Pnum=0; }
                    else { 
                        if (Pnum==0) { IN.price=0; IN.lp = 0; }
                        if (IN.lp < 6) { IN.price = IN.price * 10 + key[0] - '0'; IN.lp++; }
                        Pnum=1; } }
                else { 
                    if (type != 2) continue;
                    Pleft=0; Pnum=0;
                    if (IN.len < Mname) { 
                        LH[kpr++] = StrLenB(IN.name); strcat(IN.name, key);
                        b = Fpi(Pro,IN.name,&i); cr=i; IN.len++; } }
                continue; }
        else { switch (key[1]) {
                case K_SPA: if (IN.len == 0) continue; 
                            if (IN.name[StrLenB(IN.name) - 1] == ' ')  continue;
                            if (IN.len < Mname) {
                                strcat(IN.name, " "); IN.len++; LH[kpr++] = StrLenB(IN.name); b = Fpi(Pro, IN.name, &i); cr = i; }
                            continue;
                case K_ENT: if (IN.len && IN.price) {
                                if (b > 0) { 
                                    b = Fpi(Pro,IN.name,&i); strcpy(IN.name, Pro->dat[i].name); cr = i; }
                                else { 
                                    if (IN.name[StrLenB(IN.name) - 1] == 32) { IN.len--; IN.name[StrLenB(IN.name) - 1] = 0; }
                                    IN.name[StrLenB(IN.name)] = 0; cr = -1; }
                                if (cr == -1) { 
                                    if (AddDic(Pro, IN.name, IN.price, IN.col) == -1) { 
                                        printf("\n🚩\n"); fflush(stdout); delay_ms(1500); IN.col = -1; continue; }
                                    if (IN.len > Pro->FMN) Pro->FMN = IN.len;
                                    if (IN.lp > Pro->FMP) Pro->FMP = IN.lp; }
                                else { 
                                    if (IN.col == 0) Pro->dat[cr].qy = 0;
                                    else {
                                        IN.col = Pro->dat[cr].qy + IN.col; Pro->dat[cr].qy = (IN.col > 99) ? 99 : IN.col; }
                                    if (IN.len < Mname+1 && IN.len > Pro->FMN) Pro->FMN = IN.len;
                                    if (IN.lp > Pro->FMP) Pro->FMP = IN.lp;
                                    Pro->dat[cr].price = IN.price; }
                                IN.col = -1; }
                            continue;
                 case K_UP: Analitics(Pro); num = -1; continue;
                case K_ESC: printf(LCur Cce); fflush(stdout); tmp = SaveDic(Pro, DBase); tmp = SaveDic(Pro,DAn); tmp = SaveDic(Pro,DRep);
                            if (tmp) { printf(Cnu "%d", tmp); fflush(stdout); if (flag) SendMailSecure(DSend,DRep); }
                            SetInputMode(0); printf(ShCur); ClearDic(Pro); return;
                case K_BAC:
                case K_DEL: if (Pnum) { 
                                if (IN.lp) { IN.price /= 10; IN.lp--; }
                                continue; }
                            if (kpr > 0) {
                                kpr--; IN.name[LH[kpr]]='\0'; IN.len = StrLen(IN.name); b = Fpi(Pro,IN.name,&i); cr=i; }
                            continue;
                case K_LEF: Pleft = 1; continue;
                case K_RIG:
                case K_TAB: if (IN.len > 0) {
                                if (b > 0 && i >= 0 && i < Pro->count) {
                                    if (strcmp(IN.name, Pro->dat[i].name) == 0 && b > 1) { i++; b--; }
                                    const char *fna = Pro->dat[i].name; int cub = StrLenB(IN.name); int tb = StrLenB(fna);
                                if (tb > cub) {
                                    if (kpr < Mname) {
                                        LH[kpr++] = cub; strcpy(IN.name + cub, fna + cub); 
                                        IN.len = StrLen(IN.name); b = Fpi(Pro,IN.name, &i); cr = i; } } } }
                   default: continue; } } } }

void help() {
    printf(Cnn "Created by " Cna "Alexey Pozdnyakov" Cnn " in " Cna "01.2026" Cnn 
           " version " Cna "1.31" Cnn ", email " Cna "avp70ru@mail.ru" Cnn 
           " github " Cna "https://github.com/AVPscan" Cnn "\n"); }

int main(int argc, char *argv[]) {
    if (argc > 1) { if (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0) help();
                    return 0; }
    Dic Pro = {0};
    SWD(); Products(&Pro); return 0; }
