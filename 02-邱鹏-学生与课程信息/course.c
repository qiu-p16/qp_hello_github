/*============================================================
 * course.c  课程信息模块实现（顺序表 + MySQL）
 * 负责人：邱鹏
 *==========================================================*/
#include "../include/course.h"
#include "../include/db.h"

static Course *g_arr = NULL;     /* 动态数组 */
static int     g_len = 0;
static int     g_cap = 0;

static void ensure_cap(int need)
{
    if (need <= g_cap) return;
    int cap = g_cap == 0 ? 16 : g_cap;
    while (cap < need) cap *= 2;
    g_arr = (Course *)realloc(g_arr, (size_t)cap * sizeof(Course));
    g_cap = cap;
}

static int index_of(const char *id)
{
    for (int i = 0; i < g_len; ++i)
        if (strcmp(g_arr[i].course_id, id) == 0) return i;
    return -1;
}

int course_init(void)
{
    DbResult *rs = db_query0(
        "SELECT course_id,course_name,credit,semester FROM course ORDER BY course_id", 4);
    if (!rs) return -1;
    ensure_cap(rs->n_rows);
    for (int i = 0; i < rs->n_rows; ++i) {
        Course c;
        memset(&c, 0, sizeof(c));
        strncpy(c.course_id,   rs->data[i][0], LEN_ID-1);
        strncpy(c.course_name, rs->data[i][1], COURSE_NAME_LEN-1);
        c.credit = (float)atof(rs->data[i][2]);
        strncpy(c.semester,    rs->data[i][3], 23);
        g_arr[g_len++] = c;
    }
    db_result_free(rs);
    return 0;
}

void course_free(void)
{
    free(g_arr);
    g_arr = NULL; g_len = g_cap = 0;
}

const Course *course_find(const char *id)
{
    int i = index_of(id);
    return i >= 0 ? &g_arr[i] : NULL;
}

int course_add(const Course *c)
{
    if (index_of(c->course_id) >= 0) return -1;       /* 课程号重复 */
    MYSQL_BIND p[4];
    double credit = c->credit;
    bind_str(&p[0], c->course_id);
    bind_str(&p[1], c->course_name);
    bind_double(&p[2], &credit);
    bind_str(&p[3], c->semester);
    if (db_exec("INSERT INTO course(course_id,course_name,credit,semester) VALUES(?,?,?,?)",
                p, 4) <= 0) return -2;
    ensure_cap(g_len + 1);
    g_arr[g_len++] = *c;
    return 0;
}

int course_update(const Course *c)
{
    MYSQL_BIND p[4];
    double credit = c->credit;
    bind_str(&p[0], c->course_name);
    bind_double(&p[1], &credit);
    bind_str(&p[2], c->semester);
    bind_str(&p[3], c->course_id);
    if (db_exec("UPDATE course SET course_name=?,credit=?,semester=? WHERE course_id=?",
                p, 4) <= 0) return -1;
    int i = index_of(c->course_id);
    if (i >= 0) g_arr[i] = *c;
    return 0;
}

int course_delete(const char *id)
{
    MYSQL_BIND p[1];
    bind_str(&p[0], id);
    int n = db_exec("DELETE FROM course WHERE course_id=?", p, 1);
    if (n <= 0) {
        printf("删除失败：该课程下可能仍有成绩记录(外键约束)。\n");
        return -1;
    }
    int i = index_of(id);
    if (i >= 0) {
        for (int j = i; j < g_len - 1; ++j) g_arr[j] = g_arr[j+1];
        g_len--;
    }
    return 0;
}

void course_list_all(void)
{
    print_pad("课程号", 12); print_pad("课程名", 22);
    printf("%-8s%-10s\n", "学分", "学期");
    print_divider('-', 52);
    for (int i = 0; i < g_len; ++i) {
        print_pad(g_arr[i].course_id, 12);
        print_pad(g_arr[i].course_name, 22);
        printf("%-8.1f%-10s\n", g_arr[i].credit, g_arr[i].semester);
    }
    printf("共 %d 门课程。\n", g_len);
}

int course_count(void) { return g_len; }
