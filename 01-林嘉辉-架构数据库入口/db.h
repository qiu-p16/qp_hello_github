#ifndef DB_H
#define DB_H

/*============================================================
 * db.h  MySQL 数据库连接与访问封装
 * 负责人：林嘉辉（组长）
 * 所有 SQL 均使用预处理语句(Prepared Statement)，
 * 外部输入只通过 MYSQL_BIND 绑定，杜绝 SQL 注入。
 *==========================================================*/

#include <mysql.h>
#include "common.h"

/* ====== 数据库连接配置：按本机情况修改 ====== */
#define DB_HOST  "127.0.0.1"
#define DB_USER  "root"
#define DB_PASS  "1234"            /* 本机 MySQL 的 root 密码（实测为1234） */
#define DB_NAME  "score_db"
#define DB_PORT  3306
/* ============================================ */

#define DB_COL_MAX  512           /* 单列结果最大字符数 */

/* 查询结果集：统一转成字符串二维表，data[r][c]，SQL NULL 用 NULL 表示 */
typedef struct {
    int    n_rows;
    int    n_cols;
    char ***data;
} DbResult;

/* 连接 / 断开（程序内维护一个全局连接） */
int  db_connect(void);
void db_disconnect(void);

/* 写操作(INSERT/UPDATE/DELETE)：预处理绑定参数，返回受影响行数，-1 表示失败 */
int  db_exec(const char *sql, MYSQL_BIND *params, int n_params);
int  db_exec0(const char *sql);                 /* 无参数版本 */

/* 查询：预处理语句，返回结果集(调用方 db_result_free)，失败返回 NULL */
DbResult *db_query(const char *sql, MYSQL_BIND *params, int n_params, int n_cols);
DbResult *db_query0(const char *sql, int n_cols);   /* 无参数版本 */
void      db_result_free(DbResult *r);

/* MYSQL_BIND 参数绑定辅助(内部会清零) */
void bind_str(MYSQL_BIND *b, const char *s);
void bind_long(MYSQL_BIND *b, long *v);
void bind_double(MYSQL_BIND *b, double *v);

#endif /* DB_H */
