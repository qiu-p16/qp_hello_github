#ifndef COMMON_H
#define COMMON_H

/*============================================================
 * common.h  公共头文件：通用常量、安全输入、密码哈希、界面工具
 * 负责人：林嘉辉（组长）
 * C99 标准
 *==========================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

/* ---------------- 业务常量 ---------------- */
#define LEN_ID       24    /* 学号/课程号最大长度 */
#define LEN_NAME     52    /* 姓名最大长度 */
#define LEN_CLASS    52    /* 班级最大长度 */
#define LEN_USER     52    /* 用户名最大长度 */
#define LEN_PWD      64    /* 密码最大长度 */
#define LEN_HASH     40    /* 哈希串长度 */
#define LEN_LINE     256   /* 通用行缓冲 */
#define PAGE_SIZE    20    /* 查询结果每页条数(REQ:每页20条) */
#define PASS_SCORE   60.0  /* 及格线 */
#define EXCELLENT    85.0  /* 优秀线 */

/* 角色枚举 */
typedef enum { ROLE_GUEST = 0, ROLE_ADMIN = 1,
               ROLE_TEACHER = 2, ROLE_STUDENT = 3 } Role;

/* 当前登录用户上下文 */
typedef struct {
    char     username[LEN_USER];
    Role     role;
    char     student_id[LEN_ID];   /* 学生角色绑定学号 */
    int      logged;
} LoginUser;

/* ---------------- 控制台工具 ---------------- */
/* 设置 Windows 控制台为 UTF-8，避免中文乱码 */
void console_init(void);
void clear_screen(void);
void pause_hint(void);

/* ---------------- 安全输入 ---------------- */
/* 读取一整行字符串，自动去掉末尾换行；返回字符串指针 */
char *read_line(char *buf, int size);
/* 读取一个整数，输入非法时循环提示重输；min/max 为取值范围(不限制传 INT 范围) */
int   read_int(const char *prompt, int min, int max);
/* 读取一个浮点数 */
double read_double(const char *prompt, double min, double max);
/* 读取密码(输入不回显) */
void  read_password(char *buf, int size);

/* ---------------- 密码哈希(djb2) ---------------- */
/* 与 schema.sql 初始数据保持一致，不明文存储 */
void password_hash(const char *plain, char *out);

/* ---------------- 小工具 ---------------- */
void str_trim(char *s);                 /* 去掉首尾空白 */
int  is_blank(const char *s);           /* 是否空串 */
void print_divider(char ch, int n);     /* 打印分隔线 */
int  disp_width(const char *s);         /* 显示宽度(中文/全角算2列) */
void print_pad(const char *s, int w);   /* 按显示宽度左对齐补空格 */

#endif /* COMMON_H */
