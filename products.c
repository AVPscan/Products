#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sys.h"

#define DBase "products.txt"
#define DRep  "reports.txt"
#define DAn   "analitics.txt"
#define Mname 32
#define BufN Mname*4

char nc[BufN+1];
extern unsigned char FileBuf[DBuf+NBuf];
static int LH[128];
static int kpr = 0;

typedef struct { int n3, n2, n1, bnam, cnam, sum, tqy, qy; } PS_t;
PS_t PS;
typedef struct { 
    char* name;
    int price, qy, summa, tqy, vis; 
    int nameC,FCN,FCP,FCQ,FCS,FCT,FCV; } DicDat;
typedef struct { 
    DicDat* dat;
    int count, cap; 
    int MaxN,MaxP,MaxQ,MaxS,MaxT,MaxV; } Dic;

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
       
int char_type(const unsigned char* buf, int* len) {
    unsigned char c = *buf;
    if (c < 128) { *len = 1;
        if (c >= '0' && c <= '9') return 1;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return 2;
        if (c >= 33 && c <= 126) return 4;
        if (c == 0) return 5;
        return 3; }
    int l;
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
    
int AddDicFull(Dic* Pro, const char* name, int summa, int tqy, int qy, int mode) {
    int low = 0, cmp = 0, high = Pro->count - 1, mid, r, s;
    if (Pro->MaxN < PS.cnam) Pro->MaxN = PS.cnam;
    while (low <= high) {
        mid = (low + high) / 2;
        if ((cmp = strcmp(name, Pro->dat[mid].name)) == 0) {
            if (mode) {
                Pro->dat[mid].summa += summa; 
                Pro->dat[mid].tqy += tqy; 
                Pro->dat[mid].vis += qy;
                for (r = 1, s = Pro->dat[mid].summa; s > 9; s /= 10, r++);
                Pro->dat[mid].FCS = r; if (Pro->MaxS < r) Pro->MaxS = r;
                for (r = 1, s = Pro->dat[mid].tqy;   s > 9; s /= 10, r++);
                Pro->dat[mid].FCT = r; if (Pro->MaxT < r) Pro->MaxT = r;
                for (r = 1, s = Pro->dat[mid].vis;   s > 9; s /= 10, r++);
                Pro->dat[mid].FCV = r; if (Pro->MaxV < r) Pro->MaxV = r; }
            else { Pro->dat[mid].price = summa;
                if (tqy) Pro->dat[mid].qy += tqy;
                else Pro->dat[mid].qy = 0;
                if (Pro->dat[mid].qy > 99) Pro->dat[mid].qy=99;
                PS.tqy = 1; if (Pro->dat[mid].qy > 9) PS.tqy = 2;
                Pro->dat[mid].FCP = PS.sum; if (Pro->MaxP < PS.sum) Pro->MaxP = PS.sum;
                Pro->dat[mid].FCQ = PS.tqy; if (Pro->MaxQ < PS.tqy) Pro->MaxQ = PS.tqy; }
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
    if (mode) { Pro->dat[low].summa = summa; Pro->dat[low].tqy = tqy; Pro->dat[low].vis = qy;
                Pro->dat[low].FCS = PS.sum; Pro->dat[low].FCT = PS.tqy; Pro->dat[low].FCV = PS.qy;
                if (Pro->MaxS < Pro->dat[low].FCS) Pro->MaxS = Pro->dat[low].FCS;
                if (Pro->MaxT < Pro->dat[low].FCT) Pro->MaxT = Pro->dat[low].FCT;
                if (Pro->MaxV < Pro->dat[low].FCV) Pro->MaxV = Pro->dat[low].FCV; }
    else { if (tqy > 99) { tqy = 99; PS.tqy = 2; }
           Pro->dat[low].price = summa; Pro->dat[low].qy = tqy; Pro->dat[low].FCP = PS.sum; Pro->dat[low].FCQ = PS.tqy; 
           Pro->dat[low].summa = 0; Pro->dat[low].tqy = 0; Pro->dat[low].vis = 0; Pro->dat[low].FCS = 1; Pro->dat[low].FCT = 1; Pro->dat[low].FCV = 1;
           if (Pro->MaxQ < Pro->dat[low].FCQ) Pro->MaxQ = Pro->dat[low].FCQ;
           if (Pro->MaxP < Pro->dat[low].FCP) Pro->MaxP = Pro->dat[low].FCP; }
    Pro->count++; return low;}

int AddDic(Dic* Pro, const char* name, int summa, int tqy) { int i;
    if ( !name || name[0] == 0 || summa < 1 || tqy < 0) return -1;
    PS.sum = 1; for ( i = summa; i > 9 ; i /= 10,  PS.sum++);
    PS.tqy = 1; for ( i = tqy; i > 9 ; i /= 10,  PS.tqy++);
    PS.bnam= StringBC(name, &PS.cnam);
    return AddDicFull(Pro, name, summa, tqy, 0, 0); }

void ParseBuf( Dic* Pro, unsigned char* buf, unsigned char* out, int mode) {
    int type,len,i,j; 
      while (buf < out) {
        if ((type = char_type(buf, &len)) == 1) { while (buf < out && (type = char_type(buf, &len)) == 1 && *buf == '0') buf++ ;
                       if (type == 1) { PS.n3 = PS.n2; PS.n2 = PS.n1; PS.n1 = 0 ;
                          PS.qy = PS.tqy; PS.tqy = PS.sum; PS.sum = 0;
                          while (buf < out && (type = char_type(buf, &len)) == 1 && PS.sum < 9 ) { PS.n1 = PS.n1 * 10 + *buf - '0'; PS.sum++ ; buf++ ; } 
                          while (buf < out && (type = char_type(buf, &len)) == 1) buf++ ; } }
        if (type == 2) { PS.cnam = 0; j = 0; int sq = 0; 
            while (buf < out) { type = char_type(buf, &len);
                if (type == 1 || type == 3) break; 
                if (type == 2) { for (i = 0; i < len && j < 127; i++) nc[j++] = buf[i];
                    PS.cnam++; sq = 0; } 
                else if (type == 4) { if (j > 0 && !sq) { nc[j++] = ' '; sq = 1; PS.cnam++; } }
                buf += len; }
            if (j > 0 && nc[j-1] == ' ') { j--; PS.cnam--; }
            nc[j] = '\0'; PS.bnam = j;
            buf -= len; }
        if (PS.cnam) switch (mode) {
                        case 0: if (PS.sum) { j = AddDicFull(Pro, nc, PS.n1, 0, 0, 0); PS.tqy = 0; PS.qy = 0; }
                        case 1: if (PS.tqy) { PS.n1 *= PS.n2; PS.n3 = 1; PS.qy = 1; PS.sum = 1; for ( i = PS.n1; i > 9 ; i /= 10,  PS.sum++); }
                        case 2: if (PS.qy) j = AddDicFull(Pro, nc, PS.n1, PS.n2, PS.n3, 1);
                                PS.cnam = 0; PS.sum = 0; PS.tqy = 0;  PS.qy = 0; }
        buf+=len; } }

int LoadDic(Dic* Pro, const char* filename) {
    int i=(strcmp(filename, DBase) == 0)?0:(strncmp(filename, "rep", 3) == 0)?1:(strncmp(filename, "ana", 3) == 0)?2:3;
    if (i == 3) return -2;
    size_t Tail = 0; void* File = os_open_file(filename);
    if (!File) return -1;
    PS.cnam = 0; PS.sum = 0; PS.tqy = 0;  PS.qy = 0;
    while (1) {
        size_t LoadFLen = os_read_file(File, FileBuf + Tail, DBuf); size_t SaveFLen = LoadFLen + Tail;
        if (SaveFLen == 0) break;
        size_t SaveLen = SaveFLen; if ( LoadFLen == DBuf) { while (SaveLen > 0 && (FileBuf[SaveLen-1] > 32)) SaveLen--;
                                                            if (SaveLen == 0) SaveLen = SaveFLen; }
        ParseBuf(Pro, FileBuf, FileBuf+SaveLen, i);
        Tail = SaveFLen - SaveLen; if (Tail > 0) memmove(FileBuf, FileBuf + SaveLen, Tail);
        if ( LoadFLen < DBuf) break; }
    os_close_file(File); return Pro->count; }
    
void SaveDic(Dic* Pro, const char* filename) {
    int i = (strcmp(filename, DBase) == 0)?0:(strncmp(filename, "rep", 3) == 0)?1:(strncmp(filename, "ana", 3) == 0)?2:3; if (i == 3) return;
    void* f = os_create_file(filename); int j = 1; int s = 0;
    switch (i) {
        case 0: for (int i = 0; i < Pro->count; i++) os_print_file(f, "%*d %s\n", Pro->MaxP, Pro->dat[i].price, STU(Pro->dat[i].name));
                break;
        case 1: for (int i = 0; i < Pro->count; i++) if (Pro->dat[i].qy > 0) { s += Pro->dat[i].price * Pro->dat[i].qy;
                                                          os_print_file(f, "%02d %2d %*d %s\n", j++, Pro->dat[i].qy, Pro->MaxP, Pro->dat[i].price, STU(Pro->dat[i].name)); }
                if (s) os_print_file(f, "  %d\n", s);
                break;
        case 2: for (int i = 0; i < Pro->count; i++) if (Pro->dat[i].summa > 0) { s += Pro->dat[i].summa;
                                                          os_print_file(f, "%*d %*d %*d %s\n", Pro->MaxV, Pro->dat[i].vis,
                                                          Pro->MaxT, Pro->dat[i].tqy, Pro->MaxS, Pro->dat[i].summa, STU(Pro->dat[i].name)); }
                if (s) os_print_file(f, "  %d\n", s); }
    os_close_file(f); }
    
int PrintDic(Dic* Pro) {
    int count = 0;
    for (int i = 0; i < Pro->count; i++) { if (Pro->dat[i].qy < 1) continue;
        count++; if (Pro->dat[i].qy > 1) printf(Cnn "%02d " Cnu "%2d ", count, Pro->dat[i].qy);
                 else printf(Cnn "%02d    ", count);
        printf( Cnu "%*d " Cna "%-*s\n", Pro->MaxP, Pro->dat[i].price, Pro->MaxN + Pro->dat[i].FCN, Pro->dat[i].name); }
    return count; }
 
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
    static char res[256];
    if (i < 0 || i >= Pro->count) { sprintf(res, Cna "%s                    ", str1); return res; }
    const char *full_name = Pro->dat[i].name;
    int off = StrLenB(str1);
    sprintf(res, Cna "%s" Cnn "%s                         ", str1, full_name + off); return res; }
    
char* GetRow(Dic* Pro,int num, int price, int col, const char *name) {
    static char buffers[4][256];
    static int b_idx = 0; 
    b_idx = (b_idx + 1) & 3;
    char *d = buffers[b_idx]; 
    int c; 
    int b = StringBC(name, &c);
    int name_w = Pro->MaxN + (b - c);
    sprintf(d, Cnn "%02d " Cnu "%2d " Cnu "%*d " Cna "%-*s", num, col, Pro->MaxP, price, name_w, name );
    return d; }

void Analitics(Dic* Pro) {
    int total_all_sum = 0;
    int total_items_bought = 0;
    for (int i = 0; i < Pro->count; i++) {
        if (Pro->dat[i].tqy <= 0) continue;
        total_all_sum += Pro->dat[i].summa;
        total_items_bought += Pro->dat[i].tqy; }
    if (total_all_sum == 0 || total_items_bought == 0) return;
    // Обновленный заголовок с новой колонкой (кол-во)
    printf( Cls Cna "%-*s " Cnu "%*s " Cnu "%*s " Cnu "%*s " Cnu "📊 " Cnu "%*s\n",
           Pro->MaxN + 5, "📦", 
           Pro->MaxP, " 💸",
           Pro->MaxT, " 💱 ",
           Pro->MaxV, "🎯 ",
           Pro->MaxT, " ∑ ");             
    int j=1;
    for (int i = 0; i < Pro->count; i++) {
        if (Pro->dat[i].vis == 0) continue;
        int avg_price = Pro->dat[i].summa / Pro->dat[i].tqy;
        int frequency = (Pro->dat[i].tqy * 100) / (total_items_bought ? total_items_bought : 1);
        int share = (Pro->dat[i].summa * 100) / (total_all_sum ? total_all_sum : 1);
        const char* color = (frequency > 30) ? Cap : (frequency < 10 ? Cam : Cna);
        const char* price_status_symbol = "";
        const char* price_status_color = Cna;
        if (Pro->dat[i].price > avg_price) {
            price_status_symbol = "🚩  ";
            price_status_color = Cam; } 
        else if (Pro->dat[i].price < avg_price) {
                  price_status_symbol = "🟢  ";
                  price_status_color = Cap; } 
              else price_status_symbol = "🏳️   ";
        printf(Cnn "%02d " "%s%-*s " Cnu "%*d " "%s%3d%% " Cnu "%2d%% " "%s%s" Cnu "%*d\n",
               j++, color, Pro->MaxN + Pro->dat[i].FCN, Pro->dat[i].name,
               Pro->MaxP, avg_price,
               color, frequency, share,
               price_status_color, price_status_symbol,
               Pro->MaxT, Pro->dat[i].tqy); }
    int total_if_today = 0;
    for (int i = 0; i < Pro->count; i++) {
        if (Pro->dat[i].tqy == 0) continue;
        total_if_today += Pro->dat[i].price * Pro->dat[i].tqy; }
    int avg_item_price = total_all_sum / (total_items_bought ? total_items_bought : 1);
    printf(Cna "🏦\n");
    printf(Cnu "🧾💰 : " Cap "%d\n", avg_item_price);
    printf(Cnu "🛍️  ∑ : " Cap "%d\n", total_items_bought);
    printf(Cnu "💳 ∑ : " Cap "%d\n", total_all_sum); 
    int inflation_percent = 0;
    if (total_if_today > 0 && total_all_sum > 0) {
        inflation_percent = ((total_if_today - total_all_sum) * 100) / total_all_sum;
        const char* infla_color = (inflation_percent > 0) ? Cam : Cap;
        const char* trend_emoji = (inflation_percent > 0) ? "📈" : "📉"; 
        printf(Cnu "%s 🧐: " Cnu "%d (" "%s%+d%%" Cnu ")\n", 
               trend_emoji, total_if_today, infla_color, inflation_percent); }
    while (1) { 
        UIDelay(200);
        const char* k = GetKeyName();
        if (k && strcmp(k, "Down") == 0) break; } }

void EditDic(Dic* Pro) { 
    int b,i,cr,cl,pl,sum,Pleft,Pnum,nprice,num, ncol = -1;
    char input[129] = {0};
    while (1) {
        if (ncol == -1) { sum = 0; Pleft = 0; Pnum = 0; nprice = 0; ncol = 1; input[0] = '\0'; num = -1; kpr = 0;
                          if (!Pro || Pro->count < 1) { i = -1; b = 0; }
                          else { i = 0; b = Pro->count; }
                          cr=i; }
        if (num == -1) { printf( Cls Cna ); num = 1 + PrintDic(Pro); printf(SCur); }
        cl = StrLen(input); if (cl > Pro->MaxN) Pro->MaxN = cl;
        if (cr!=-1) { if (sum==0) nprice=Pro->dat[cr].price;
                      else nprice=sum; }
        printf(LCur "%s",GetRow(Pro,num,nprice,ncol,prw(Pro,input,cr))); fflush(stdout);
        UIDelay(b == 0?100:(b == 1?80:60));
        const char *raw_key = GetKeyName();
        char key_buf[64] = {0};
        if (raw_key && *raw_key) { strncpy(key_buf, raw_key, 63); char *key = key_buf;
            if (strcmp(key, "Space") == 0) { key[0] = ' '; key[1] = '\0'; }
            if (strcmp(key, "Enter") == 0) { if (kpr > 0 && nprice!=0) {
                                                if (b > 0) { b = Fpi(Pro,input,&i); cr = i; strcpy(input, Pro->dat[i].name); }
                                                else cr = -1;
                                                cl = StrLen(input); if (cl < 33 && cl > Pro->MaxN) Pro->MaxN = cl;
                                                printf(LCur "%s",GetRow(Pro,num,nprice,ncol,input)); fflush(stdout);
                                                if (cr == -1) { if (AddDic(Pro, input, nprice, ncol) == -1) { printf("\n🚩\n"); fflush(stdout); UIDelay(1500); }
                                                                ncol = -1; continue; }
                                                else { if (ncol == 0) Pro->dat[cr].qy = 0;
                                                       else { ncol = Pro->dat[cr].qy + ncol; Pro->dat[cr].qy = (ncol > 99) ? 99 : ncol; }
                                                       Pro->dat[cr].price = nprice; }
                                                ncol = -1; }
                                              continue; }
            if (strcmp(key, "Up") == 0) { Analitics(Pro); num = -1; continue; }
            if (strcmp(key, "Escape") == 0) { printf(LCur Cce); fflush(stdout); return; }
            if (strcmp(key, "Backspace") == 0 || strcmp(key, "Delete") == 0) {
                if (kpr>0) { kpr--; input[LH[kpr]]='\0'; b = Fpi(Pro,input,&i); cr=i; }
                else sum=0;
                continue; }
            if (strcmp(key, "Left") == 0) { Pleft=1; continue; }
            if (strcmp(key, "Right") == 0 || strcmp(key, "Tab") == 0) { 
                if (b > 0 && i >= 0 && i < Pro->count) {
                    if (strcmp(input, Pro->dat[i].name) == 0 && b > 1) { i++; b--; }
                    const char *fna = Pro->dat[i].name;
                    int cub = StrLenB(input);
                    int tb = StrLenB(fna);
                    if (tb > cub) { if (kpr < 128) LH[kpr++] = cub;
                        strcpy(input + cub, fna + cub);
                        b = Fpi(Pro,input, &i);
                        cr = i; } }
                continue; }
            if (StrLen(key) == 1) {
                if (key[0]>='0' && key[0]<='9') { if (Pleft ) { ncol=key[0]-'0'; Pleft=0; Pnum=0; } 
                                                  else { if (Pnum==0) sum=0;
                                                          pl = 1; for (Pnum = sum; Pnum > 9 ; Pnum /= 10, pl++);
                                                          if (pl < 6) sum = sum * 10 + key[0] - '0';
                                                          if (Pro->MaxP < pl) Pro->MaxP = pl; 
                                                          nprice=sum; Pnum=1; }
                                                  continue; }
                else { Pleft=0; Pnum=0; if (StrLenB(input)+ StrLenB(key) < 128) { LH[kpr++] = StrLenB(input); strcat(input, key);
                                                                                  b = Fpi(Pro,input,&i); cr=i; } } } }
        if (b <= 0) cr = -1;
        else { cr = (cr - i + 1) % b; if (cr < 0) cr += b;
               cr += i; } } }

void Products(void) {
    Dic Pro = {0}; int opt;
    SetInputMode(1); printf(HCur); opt = LoadDic(&Pro, DBase); opt = LoadDic(&Pro, DAn); opt = LoadDic(&Pro, DRep);
    EditDic(&Pro); SaveDic(&Pro, DBase); SaveDic(&Pro,DAn); SaveDic(&Pro,DRep); opt = 0; 
    for (int i = 0; i < Pro.count; i++) if (Pro.dat[i].qy > 0) opt += Pro.dat[i].price * Pro.dat[i].qy;
    if (opt) { printf(Cnu "%d", opt); fflush(stdout); }
    SetInputMode(0); printf(ShCur); ClearDic(&Pro); }
    
void help() {
    printf(Cnn "Created by " Cna "Alexey Pozdnyakov" Cnn " in " Cna "01.2026" Cnn 
           " version " Cna "1.02" Cnn ", email " Cna "avp70ru@mail.ru" Cnn 
           " github " Cna "https://github.com/AVPscan" Cnn "\n"); }

int main(int argc, char *argv[]) {
    if (argc > 1) if (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0) { help(); return 0; }
    SWD(); Products(); return 0; }
