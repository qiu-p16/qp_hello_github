/*============================================================
 * common.c  公共工具实现
 * 负责人：林嘉辉（组长）
 *==========================================================*/
#include "../include/common.h"
#include <conio.h>
#include <io.h>
#include <limits.h>

void console_init(void)
{
    SetConsoleOutputCP(65001);   /* 控制台 UTF-8 */
    SetConsoleCP(65001);
}

void clear_screen(void)
{
    system("cls");
}

void pause_hint(void)
{
    printf("\n按回车键继续...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
    getchar();
}

char *read_line(char *buf, int size)
{
    if (fgets(buf, size, stdin) == NULL) {
        buf[0] = '\0';
        return buf;
    }
    size_t len = strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
        buf[--len] = '\0';
    }
    return buf;
}

int read_int(const char *prompt, int min, int max)
{
    char line[64];
    long v;
    char *end;
    for (;;) {
        printf("%s", prompt);
        read_line(line, sizeof(line));
        v = strtol(line, &end, 10);
        while (*end == ' ') end++;          /* 允许尾部空格 */
        if (*end != '\0' || v < min || v > max) {
            printf("  输入无效，请输入 %d ~ %d 之间的整数。\n", min, max);
            continue;
        }
        return (int)v;
    }
}

double read_double(const char *prompt, double min, double max)
{
    char line[64];
    double v;
    char *end;
    for (;;) {
        printf("%s", prompt);
        read_line(line, sizeof(line));
        v = strtod(line, &end);
        while (*end == ' ') end++;
        if (*end != '\0' || v < min || v > max) {
            printf("  输入无效，请输入 %.2f ~ %.2f 之间的数值。\n", min, max);
            continue;
        }
        return v;
    }
}

/* 逐字符读取，不回显密码(退格可删)；非控制台(管道/重定向)时按行读取 */
void read_password(char *buf, int size)
{
    if (!_isatty(_fileno(stdin))) { read_line(buf, size); return; }
    int i = 0, ch;
    while ((ch = _getch()) != '\r' && ch != '\n') {
        if (ch == '\b') {                 /* 退格 */
            if (i > 0) { i--; printf("\b \b"); }
        } else if (i < size - 1 && ch >= 32) {
            buf[i++] = (char)ch;
            printf("*");
        }
    }
    buf[i] = '\0';
    printf("\n");
}

/* djb2 哈希，输出16位十六进制字符串 */
void password_hash(const char *plain, char *out)
{
    unsigned long long h = 5381ULL;
    for (const char *p = plain; *p; ++p) {
        h = ((h << 5) + h) + (unsigned char)(*p);   /* h*33 + c */
    }
    sprintf(out, "%016llx", h);
}

void str_trim(char *s)
{
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) s[--len] = '\0';
}

int is_blank(const char *s)
{
    if (!s) return 1;
    while (*s) { if (!isspace((unsigned char)*s)) return 0; s++; }
    return 1;
}

void print_divider(char ch, int n)
{
    for (int i = 0; i < n; ++i) putchar(ch);
    putchar('\n');
}

/* UTF-8 字符串在中文控制台的显示宽度：ASCII占1列，其余宽字符占2列 */
int disp_width(const char *s)
{
    int w = 0;
    const unsigned char *p = (const unsigned char *)s;
    while (*p) {
        unsigned char b = *p;
        if (b < 0x80) { w += 1; p += 1; }
        else {
            int n = 1;
            if      ((b & 0xE0) == 0xC0) n = 2;
            else if ((b & 0xF0) == 0xE0) n = 3;
            else if ((b & 0xF8) == 0xF0) n = 4;
            w += 2;
            p += n;
        }
    }
    return w;
}

/* 左对齐输出并按显示宽度补空格，解决中文列表错位 */
void print_pad(const char *s, int w)
{
    int cur = disp_width(s);
    printf("%s", s);
    for (int i = cur; i < w; ++i) putchar(' ');
}
