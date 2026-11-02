/*============================================================
 * score.c  成绩管理模块实现（链表 + 快速排序 + MySQL）
 * 负责人：孙汶
 *==========================================================*/
#include "../include/score.h"
#include "../include/db.h"

/* ---------------- 结果集工具 ---------------- */
void score_set_free(ScoreSet *s)
{
    if (!s) return;
    free(s->items);
    free(s);
}

/* 把 DbResult(7列) 转成 ScoreSet */
static ScoreSet *to_set(DbResult *rs)
{
    if (!rs) return NULL;
    ScoreSet *s = (ScoreSet *)calloc(1, sizeof(ScoreSet));
    s->n = rs->n_rows;
    s->items = (ScoreRec *)calloc((size_t)s->n + 1, sizeof(ScoreRec));
    for (int i = 0; i < rs->n_rows; ++i) {
        ScoreRec *r = &s->items[i];
        strncpy(r->student_id,  rs->data[i][0], LEN_ID-1);
        strncpy(r->name,        rs->data[i][1], LEN_NAME-1);
        strncpy(r->class_name,  rs->data[i][2], LEN_CLASS-1);
        strncpy(r->course_id,   rs->data[i][3], LEN_ID-1);
        strncpy(r->course_name, rs->data[i][4], COURSE_NAME_LEN-1);
        r->score = atof(rs->data[i][5]);
        if (rs->data[i][6]) strncpy(r->semester, rs->data[i][6], 23);
    }
    db_result_free(rs);
    return s;
}

/* ---------------- 增删改 ---------------- */
int score_add(const char *sid, const char *cid, double score, const char *sem)
{
    if (score < 0 || score > 100) {
        printf("成绩必须在 0~100 之间。\n"); return -1;
    }
    if (!student_find(sid))   { printf("学号不存在，请先录入学生。\n"); return -2; }
    if (!course_find(cid))    { printf("课程号不存在，请先录入课程。\n"); return -3; }
    MYSQL_BIND p[4];
    bind_str(&p[0], sid);
    bind_str(&p[1], cid);
    bind_double(&p[2], &score);
    bind_str(&p[3], is_blank(sem) ? "" : sem);
    int n = db_exec(
        "INSERT INTO score(student_id,course_id,score,semester) VALUES(?,?,?,?)",
        p, 4);
    if (n <= 0) { printf("录入失败：该生该课程成绩可能已存在(联合唯一)。\n"); return -4; }
    printf("成绩录入成功。\n");
    return 0;
}

int score_update(const char *sid, const char *cid, double score)
{
    if (score < 0 || score > 100) { printf("成绩必须在 0~100 之间。\n"); return -1; }
    MYSQL_BIND p[3];
    bind_double(&p[0], &score);
    bind_str(&p[1], sid);
    bind_str(&p[2], cid);
    int n = db_exec("UPDATE score SET score=? WHERE student_id=? AND course_id=?", p, 3);
    printf(n > 0 ? "修改成功。\n" : "未找到对应成绩记录。\n");
    return n > 0 ? 0 : -1;
}

int score_delete(const char *sid, const char *cid)
{
    MYSQL_BIND p[2];
    bind_str(&p[0], sid);
    bind_str(&p[1], cid);
    int n = db_exec("DELETE FROM score WHERE student_id=? AND course_id=?", p, 2);
    printf(n > 0 ? "删除成功。\n" : "未找到对应成绩记录。\n");
    return n > 0 ? 0 : -1;
}

/* ---------------- 多条件查询(AND语义，值全部绑定) ---------------- */
ScoreSet *score_query(const char *sid, const char *cid,
                      const char *cls, const char *sem)
{
    char sql[1024];
    strcpy(sql,
        "SELECT s.student_id,st.name,st.class_name,s.course_id,c.course_name,"
        "s.score,IFNULL(s.semester,c.semester) "
        "FROM score s "
        "JOIN student st ON s.student_id=st.student_id "
        "JOIN course  c  ON s.course_id=c.course_id WHERE 1=1");
    if (!is_blank(sid))  strcat(sql, " AND s.student_id=?");
    if (!is_blank(cid))  strcat(sql, " AND s.course_id=?");
    if (!is_blank(cls))  strcat(sql, " AND st.class_name=?");
    if (!is_blank(sem))  strcat(sql, " AND IFNULL(s.semester,c.semester)=?");

    MYSQL_BIND binds[4];
    int k = 0;
    if (!is_blank(sid))  bind_str(&binds[k++], sid);
    if (!is_blank(cid))  bind_str(&binds[k++], cid);
    if (!is_blank(cls))  bind_str(&binds[k++], cls);
    if (!is_blank(sem))  bind_str(&binds[k++], sem);
    return to_set(db_query(sql, binds, k, 7));
}

ScoreSet *score_query_by_student(const char *sid)
{
    return score_query(sid, "", "", "");
}

/* ---------------- 快速排序 ---------------- */
static int less_than(const ScoreRec *x, const ScoreRec *y, int by_score, int desc)
{
    int cmp;
    if (by_score) {
        cmp = (x->score < y->score) ? -1 : (x->score > y->score ? 1 : 0);
        if (cmp == 0) cmp = strcmp(x->student_id, y->student_id);
    } else {
        cmp = strcmp(x->student_id, y->student_id);
    }
    return desc ? (cmp > 0) : (cmp < 0);
}

static void swap(ScoreRec *a, ScoreRec *b)
{ ScoreRec t = *a; *a = *b; *b = t; }

void score_quicksort(ScoreRec *a, int lo, int hi, int by_score, int desc)
{
    if (lo >= hi) return;
    ScoreRec pivot = a[(lo + hi) / 2];
    int i = lo, j = hi;
    while (i <= j) {
        while (less_than(&a[i], &pivot, by_score, desc)) i++;
        while (less_than(&pivot, &a[j], by_score, desc)) j--;
        if (i <= j) { swap(&a[i], &a[j]); i++; j--; }
    }
    if (lo < j) score_quicksort(a, lo, j, by_score, desc);
    if (i < hi) score_quicksort(a, i, hi, by_score, desc);
}

/* ---------------- 单向链表 ---------------- */
SNode *score_list_build(const ScoreSet *s)
{
    SNode *head = NULL, *tail = NULL;
    for (int i = 0; i < s->n; ++i) {
        SNode *node = (SNode *)calloc(1, sizeof(SNode));
        node->rec = s->items[i];
        if (!head) head = tail = node;
        else { tail->next = node; tail = node; }
    }
    return head;
}

void score_list_print(SNode *head)
{
    print_pad("学号", 15); print_pad("姓名", 10);
    print_pad("课程号", 10); print_pad("课程名", 18);
    printf("%-7s%-8s\n", "成绩", "学期");
    print_divider('-', 68);
    int idx = 0;
    for (SNode *p = head; p; p = p->next) {
        ScoreRec *r = &p->rec;
        print_pad(r->student_id, 15); print_pad(r->name, 10);
        print_pad(r->course_id, 10);  print_pad(r->course_name, 18);
        printf("%-7.1f%-8s\n", r->score, r->semester);
        ++idx;
    }
    printf("共 %d 条记录。\n", idx);
}

void score_list_free(SNode *head)
{
    while (head) { SNode *nx = head->next; free(head); head = nx; }
}

/* 排序 + 分页展示 */
void score_show_paged(ScoreSet *s, int by_score, int desc)
{
    if (!s || s->n == 0) { printf("没有符合条件的记录。\n"); return; }
    score_quicksort(s->items, 0, s->n - 1, by_score, desc);
    SNode *head = score_list_build(s);     /* 结果以链表承载后遍历 */
    SNode *p = head;
    int total = s->n, shown = 0, page = 1;
    while (p) {
        printf("\n---- 第 %d 页 ----\n", page);
        print_pad("学号", 15); print_pad("姓名", 10);
        print_pad("课程号", 10); print_pad("课程名", 18);
        printf("%-7s%-8s\n", "成绩", "学期");
        print_divider('-', 68);
        int cnt = 0;
        while (p && cnt < PAGE_SIZE) {
            ScoreRec *r = &p->rec;
            print_pad(r->student_id, 15); print_pad(r->name, 10);
            print_pad(r->course_id, 10);  print_pad(r->course_name, 18);
            printf("%-7.1f%-8s\n", r->score, r->semester);
            p = p->next; cnt++; shown++;
        }
        printf("已显示 %d/%d 条。\n", shown, total);
        if (p) {
            printf("回车查看下一页，输入 q 返回：");
            char line[16]; read_line(line, sizeof(line));
            if (line[0] == 'q' || line[0] == 'Q') break;
        }
        page++;
    }
    score_list_free(head);
}

/* ---------------- 统计分析 ---------------- */
void score_stat(const char *cid, const char *cls, const char *sem)
{
    ScoreSet *s = score_query("", cid, cls, sem);
    if (!s || s->n == 0) { printf("没有可统计的数据。\n"); score_set_free(s); return; }
    double sum = 0, mx = -1, mn = 101;
    int pass = 0, excellent = 0;
    for (int i = 0; i < s->n; ++i) {
        double v = s->items[i].score;
        sum += v;
        if (v > mx) mx = v;
        if (v < mn) mn = v;
        if (v >= PASS_SCORE) pass++;
        if (v >= EXCELLENT)  excellent++;
    }
    int n = s->n;
    printf("\n===== 成绩统计分析（%d条）=====\n", n);
    printf("平均分  ：%.2f\n", sum / n);
    printf("最高分  ：%.2f\n", mx);
    printf("最低分  ：%.2f\n", mn);
    printf("及格率  ：%.1f%%（%d/%d，及格线%.0f）\n", 100.0*pass/n, pass, n, PASS_SCORE);
    printf("优秀率  ：%.1f%%（%d/%d，优秀线%.0f）\n", 100.0*excellent/n, excellent, n, EXCELLENT);
    score_set_free(s);
}

/* ---------------- 标准竞赛排名 ---------------- */
void score_rank(const char *cid, const char *sem)
{
    ScoreSet *s = score_query("", cid, "", sem);
    if (!s || s->n == 0) { printf("没有可排名的数据。\n"); score_set_free(s); return; }
    score_quicksort(s->items, 0, s->n - 1, 1, 1);   /* 按成绩降序 */
    printf("\n===== 成绩排名（标准竞赛排名，同分同名次）=====\n");
    printf("%-6s", "名次"); print_pad("学号", 15);
    print_pad("姓名", 10);  print_pad("课程名", 18);
    printf("%-7s\n", "成绩");
    print_divider('-', 56);
    int rank = 0;
    double last = -1;
    for (int i = 0; i < s->n; ++i) {
        ScoreRec *r = &s->items[i];
        if (i == 0 || r->score != last) { rank = i + 1; last = r->score; }
        printf("%-6d", rank); print_pad(r->student_id, 15);
        print_pad(r->name, 10); print_pad(r->course_name, 18);
        printf("%-7.1f\n", r->score);
    }
    score_set_free(s);
}
