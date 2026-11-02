/*============================================================
 * student.c  学生信息模块实现（BST + MySQL）
 * 负责人：邱鹏
 *==========================================================*/
#include "../include/student.h"
#include "../include/db.h"

static StuNode *g_root = NULL;
static int      g_count = 0;

/* ---------------- BST 内部操作 ---------------- */
static StuNode *bst_insert(StuNode *root, const Student *s, int *ok)
{
    if (!root) {
        StuNode *n = (StuNode *)calloc(1, sizeof(StuNode));
        n->data = *s;
        *ok = 1;
        return n;
    }
    int cmp = strcmp(s->student_id, root->data.student_id);
    if (cmp < 0)       root->left  = bst_insert(root->left, s, ok);
    else if (cmp > 0)  root->right = bst_insert(root->right, s, ok);
    else               *ok = 0;                 /* 学号重复 */
    return root;
}

static StuNode *bst_search(StuNode *root, const char *sid)
{
    if (!root) return NULL;
    int cmp = strcmp(sid, root->data.student_id);
    if (cmp == 0) return root;
    return bst_search(cmp < 0 ? root->left : root->right, sid);
}

/* 找最小节点(最左下) */
static StuNode *bst_min(StuNode *root)
{
    while (root && root->left) root = root->left;
    return root;
}

static StuNode *bst_remove(StuNode *root, const char *sid, int *ok)
{
    if (!root) { *ok = 0; return NULL; }
    int cmp = strcmp(sid, root->data.student_id);
    if (cmp < 0) {
        root->left = bst_remove(root->left, sid, ok);
    } else if (cmp > 0) {
        root->right = bst_remove(root->right, sid, ok);
    } else {
        *ok = 1;
        StuNode *tmp;
        if (!root->left) { tmp = root->right; free(root); return tmp; }
        if (!root->right){ tmp = root->left;  free(root); return tmp; }
        /* 双子节点：用右子树最小节点替换 */
        tmp = bst_min(root->right);
        root->data = tmp->data;
        int dummy;
        root->right = bst_remove(root->right, tmp->data.student_id, &dummy);
    }
    return root;
}

static void bst_inorder(StuNode *root, int *idx, const char *filter_class)
{
    if (!root) return;
    bst_inorder(root->left, idx, filter_class);
    const Student *s = &root->data;
    if (!filter_class || strcmp(s->class_name, filter_class) == 0) {
        print_pad(s->student_id, 15);
        print_pad(s->name, 10);
        print_pad(s->class_name, 18);
        print_pad(s->gender, 6);
        printf("%d\n", s->age);
        ++(*idx);
    }
    bst_inorder(root->right, idx, filter_class);
}

static void bst_free(StuNode *root)
{
    if (!root) return;
    bst_free(root->left);
    bst_free(root->right);
    free(root);
}

/* ---------------- 对外接口 ---------------- */
int student_init(void)
{
    DbResult *rs = db_query0(
        "SELECT student_id,name,class_name,IFNULL(gender,'男'),IFNULL(age,18) FROM student", 5);
    if (!rs) return -1;
    for (int i = 0; i < rs->n_rows; ++i) {
        Student s;
        memset(&s, 0, sizeof(s));
        strncpy(s.student_id, rs->data[i][0], LEN_ID-1);
        strncpy(s.name,       rs->data[i][1], LEN_NAME-1);
        strncpy(s.class_name, rs->data[i][2], LEN_CLASS-1);
        strncpy(s.gender,     rs->data[i][3], 3);
        s.age = atoi(rs->data[i][4]);
        int ok = 0;
        g_root = bst_insert(g_root, &s, &ok);
        if (ok) g_count++;
    }
    db_result_free(rs);
    return 0;
}

void student_free(void) { bst_free(g_root); g_root = NULL; g_count = 0; }

const Student *student_find(const char *sid)
{
    StuNode *n = bst_search(g_root, sid);
    return n ? &n->data : NULL;
}

int student_add(const Student *s)
{
    if (student_find(s->student_id)) return -1;      /* 学号已存在 */
    MYSQL_BIND p[5];
    long age = s->age;
    bind_str(&p[0], s->student_id);
    bind_str(&p[1], s->name);
    bind_str(&p[2], s->class_name);
    bind_str(&p[3], s->gender);
    bind_long(&p[4], &age);
    if (db_exec("INSERT INTO student(student_id,name,class_name,gender,age) VALUES(?,?,?,?,?)",
                p, 5) <= 0) return -2;
    int ok = 0;
    g_root = bst_insert(g_root, s, &ok);
    if (ok) g_count++;
    return 0;
}

int student_update(const Student *s)
{
    MYSQL_BIND p[5];
    long age = s->age;
    bind_str(&p[0], s->name);
    bind_str(&p[1], s->class_name);
    bind_str(&p[2], s->gender);
    bind_long(&p[3], &age);
    bind_str(&p[4], s->student_id);
    if (db_exec("UPDATE student SET name=?,class_name=?,gender=?,age=? WHERE student_id=?",
                p, 5) <= 0) return -1;
    StuNode *n = bst_search(g_root, s->student_id);
    if (n) n->data = *s;
    return 0;
}

int student_delete(const char *sid)
{
    MYSQL_BIND p[1];
    bind_str(&p[0], sid);
    int n = db_exec("DELETE FROM student WHERE student_id=?", p, 1);
    if (n <= 0) {
        printf("删除失败：该学生可能仍存在成绩记录(外键约束)，请先删除其成绩。\n");
        return -1;
    }
    int ok = 0;
    g_root = bst_remove(g_root, sid, &ok);
    if (ok) g_count--;
    return 0;
}

static void print_header(void)
{
    print_pad("学号", 15); print_pad("姓名", 10);
    print_pad("班级", 18); print_pad("性别", 6);
    printf("年龄\n");
    print_divider('-', 53);
}

void student_list_all(void)
{
    print_header();
    int idx = 0;
    bst_inorder(g_root, &idx, NULL);
    printf("共 %d 名学生。\n", idx);
}

void student_list_by_class(const char *class_name)
{
    print_header();
    int idx = 0;
    bst_inorder(g_root, &idx, class_name);
    printf("班级[%s]共 %d 名学生。\n", class_name, idx);
}

int student_count(void) { return g_count; }
