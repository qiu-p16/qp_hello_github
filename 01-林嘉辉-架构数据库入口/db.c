/*============================================================
 * db.c  MySQL 访问封装实现（预处理语句）
 * 负责人：林嘉辉（组长）
 *==========================================================*/
#include "../include/db.h"
#include <stdbool.h>

static MYSQL *g_conn = NULL;

int db_connect(void)
{
    if (g_conn) return 0;
    g_conn = mysql_init(NULL);
    if (!g_conn) {
        fprintf(stderr, "mysql_init 失败\n");
        return -1;
    }
    /* 设置字符集（MySQL 9 已废弃 MYSQL_OPT_RECONNECT，不再设置） */
    mysql_options(g_conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    if (!mysql_real_connect(g_conn, DB_HOST, DB_USER, DB_PASS,
                            DB_NAME, DB_PORT, NULL, 0)) {
        fprintf(stderr, "数据库连接失败：%s\n", mysql_error(g_conn));
        fprintf(stderr, "请确认：1.MySQL服务已启动 2.已执行schema.sql 3.db.h中账号密码正确\n");
        mysql_close(g_conn);
        g_conn = NULL;
        return -1;
    }
    mysql_set_character_set(g_conn, "utf8mb4");
    return 0;
}

void db_disconnect(void)
{
    if (g_conn) {
        mysql_close(g_conn);
        g_conn = NULL;
    }
}

/* ---------- 参数绑定辅助 ---------- */
void bind_str(MYSQL_BIND *b, const char *s)
{
    memset(b, 0, sizeof(MYSQL_BIND));
    b->buffer_type   = MYSQL_TYPE_STRING;
    b->buffer        = (void *)s;
    b->buffer_length = (unsigned long)(s ? strlen(s) : 0);
}
void bind_long(MYSQL_BIND *b, long *v)
{
    memset(b, 0, sizeof(MYSQL_BIND));
    b->buffer_type = MYSQL_TYPE_LONGLONG;
    b->buffer      = (void *)v;
    b->is_unsigned = 0;
}
void bind_double(MYSQL_BIND *b, double *v)
{
    memset(b, 0, sizeof(MYSQL_BIND));
    b->buffer_type = MYSQL_TYPE_DOUBLE;
    b->buffer      = (void *)v;
}

/* 准备语句并绑定参数、执行；返回 stmt，失败返回 NULL */
static MYSQL_STMT *prepare_and_run(const char *sql, MYSQL_BIND *params, int n)
{
    MYSQL_STMT *stmt = mysql_stmt_init(g_conn);
    if (!stmt) return NULL;
    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0) {
        fprintf(stderr, "SQL预处理失败：%s\nSQL:%s\n", mysql_stmt_error(stmt), sql);
        mysql_stmt_close(stmt);
        return NULL;
    }
    if (params && n > 0) {
        if (mysql_stmt_bind_param(stmt, params) != 0) {
            fprintf(stderr, "参数绑定失败：%s\n", mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            return NULL;
        }
    }
    if (mysql_stmt_execute(stmt) != 0) {
        fprintf(stderr, "SQL执行失败：%s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return NULL;
    }
    return stmt;
}

int db_exec(const char *sql, MYSQL_BIND *params, int n_params)
{
    MYSQL_STMT *stmt = prepare_and_run(sql, params, n_params);
    if (!stmt) return -1;
    int affected = (int)mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);
    return affected;
}

int db_exec0(const char *sql)
{
    return db_exec(sql, NULL, 0);
}

DbResult *db_query(const char *sql, MYSQL_BIND *params, int n_params, int n_cols)
{
    MYSQL_STMT *stmt = prepare_and_run(sql, params, n_params);
    if (!stmt) return NULL;

    /* 为每一列准备字符串结果缓冲 */
    MYSQL_BIND *rb = (MYSQL_BIND *)calloc((size_t)n_cols, sizeof(MYSQL_BIND));
    char **bufs = (char **)calloc((size_t)n_cols, sizeof(char *));
    unsigned long *lens = (unsigned long *)calloc((size_t)n_cols, sizeof(unsigned long));
    bool *nulls = (bool *)calloc((size_t)n_cols, sizeof(bool));
    bool *errs  = (bool *)calloc((size_t)n_cols, sizeof(bool));
    for (int c = 0; c < n_cols; ++c) {
        bufs[c] = (char *)calloc(DB_COL_MAX, 1);
        memset(&rb[c], 0, sizeof(MYSQL_BIND));
        rb[c].buffer_type   = MYSQL_TYPE_STRING;
        rb[c].buffer        = bufs[c];
        rb[c].buffer_length = DB_COL_MAX;
        rb[c].length        = &lens[c];
        rb[c].is_null       = &nulls[c];
        rb[c].error         = &errs[c];
    }
    mysql_stmt_bind_result(stmt, rb);
    mysql_stmt_store_result(stmt);

    /* 动态收集行 */
    int cap = 16, rows = 0;
    char ***data = (char ***)calloc((size_t)cap, sizeof(char **));
    int rc;
    while ((rc = mysql_stmt_fetch(stmt)) == 0 || rc == MYSQL_DATA_TRUNCATED) {
        if (rows >= cap) {
            cap *= 2;
            data = (char ***)realloc(data, (size_t)cap * sizeof(char **));
        }
        char **row = (char **)calloc((size_t)n_cols, sizeof(char *));
        for (int c = 0; c < n_cols; ++c) {
            if (nulls[c]) {
                row[c] = NULL;
            } else {
                row[c] = strdup(bufs[c]);
            }
        }
        data[rows++] = row;
    }

    DbResult *res = (DbResult *)calloc(1, sizeof(DbResult));
    res->n_rows = rows;
    res->n_cols = n_cols;
    res->data   = data;

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    for (int c = 0; c < n_cols; ++c) free(bufs[c]);
    free(bufs); free(lens); free(nulls); free(errs); free(rb);
    return res;
}

DbResult *db_query0(const char *sql, int n_cols)
{
    return db_query(sql, NULL, 0, n_cols);
}

void db_result_free(DbResult *r)
{
    if (!r) return;
    for (int i = 0; i < r->n_rows; ++i) {
        for (int c = 0; c < r->n_cols; ++c) free(r->data[i][c]);
        free(r->data[i]);
    }
    free(r->data);
    free(r);
}
