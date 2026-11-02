/*============================================================
 * auth.c  用户与权限模块实现（哈希表 + MySQL 持久化）
 * 负责人：孙汶
 *==========================================================*/
#include "../include/auth.h"
#include "../include/db.h"

static User *g_table[HASH_SIZE];

/* djb2 字符串哈希后取模定位桶 */
static unsigned int hash_index(const char *key)
{
    unsigned long h = 5381UL;
    for (; *key; ++key) h = ((h << 5) + h) + (unsigned char)(*key);
    return (unsigned int)(h % HASH_SIZE);
}

/* 头插法加入哈希表 */
static void hash_insert(User *u)
{
    unsigned int idx = hash_index(u->username);
    u->next = g_table[idx];
    g_table[idx] = u;
}

static User *new_user(const char *name, const char *hash,
                      Role role, const char *sid)
{
    User *u = (User *)calloc(1, sizeof(User));
    strncpy(u->username, name, LEN_USER - 1);
    strncpy(u->pwd_hash, hash, LEN_HASH - 1);
    u->role = role;
    if (sid) strncpy(u->student_id, sid, LEN_ID - 1);
    return u;
}

const char *role_to_str(Role r)
{
    switch (r) {
        case ROLE_ADMIN:   return "admin";
        case ROLE_TEACHER: return "teacher";
        case ROLE_STUDENT: return "student";
        default:           return "guest";
    }
}
Role role_from_str(const char *s)
{
    if (strcmp(s, "admin") == 0)   return ROLE_ADMIN;
    if (strcmp(s, "teacher") == 0) return ROLE_TEACHER;
    if (strcmp(s, "student") == 0) return ROLE_STUDENT;
    return ROLE_GUEST;
}

int auth_init(void)
{
    DbResult *rs = db_query0(
        "SELECT username,password,role,IFNULL(student_id,'') FROM user", 4);
    if (!rs) return -1;
    for (int i = 0; i < rs->n_rows; ++i) {
        Role r = role_from_str(rs->data[i][2]);
        hash_insert(new_user(rs->data[i][0], rs->data[i][1], r, rs->data[i][3]));
    }
    db_result_free(rs);
    return 0;
}

void auth_free(void)
{
    for (int i = 0; i < HASH_SIZE; ++i) {
        User *u = g_table[i];
        while (u) { User *nx = u->next; free(u); u = nx; }
        g_table[i] = NULL;
    }
}

User *auth_find(const char *username)
{
    User *u = g_table[hash_index(username)];
    while (u) {
        if (strcmp(u->username, username) == 0) return u;
        u = u->next;
    }
    return NULL;
}

int auth_login(const char *username, const char *pwd, LoginUser *out)
{
    User *u = auth_find(username);
    if (!u) return -1;                         /* 用户不存在 */
    char hash[LEN_HASH];
    password_hash(pwd, hash);
    if (strcmp(hash, u->pwd_hash) != 0) return -2;  /* 密码错误 */
    strncpy(out->username, u->username, LEN_USER - 1);
    out->role = u->role;
    strncpy(out->student_id, u->student_id, LEN_ID - 1);
    out->logged = 1;
    return 0;
}

/* 内部：写库 + 入表，调用方已查重 */
static int create_user(const char *name, const char *pwd,
                       Role role, const char *sid)
{
    char hash[LEN_HASH];
    password_hash(pwd, hash);
    MYSQL_BIND p[4];
    bind_str(&p[0], name);
    bind_str(&p[1], hash);
    bind_str(&p[2], role_to_str(role));
    bind_str(&p[3], (sid && !is_blank(sid)) ? sid : "");
    int n = db_exec(
        "INSERT INTO user(username,password,role,student_id) VALUES(?,?,?,?)",
        p, 4);
    if (n <= 0) return -1;
    hash_insert(new_user(name, hash, role, sid));
    return 0;
}

int auth_register(const char *username, const char *pwd,
                  Role role, const char *student_id)
{
    if (auth_find(username)) return -1;       /* 重名 */
    return create_user(username, pwd, role, student_id);
}

int auth_add_user(const char *username, const char *pwd,
                  Role role, const char *student_id)
{
    return auth_register(username, pwd, role, student_id);
}

int auth_change_password(const char *username, const char *old_pwd,
                         const char *new_pwd)
{
    User *u = auth_find(username);
    if (!u) return -1;
    char old_hash[LEN_HASH], new_hash[LEN_HASH];
    password_hash(old_pwd, old_hash);
    if (strcmp(old_hash, u->pwd_hash) != 0) return -2;   /* 原密码错 */
    password_hash(new_pwd, new_hash);
    if (strcmp(new_hash, u->pwd_hash) == 0) return -3;   /* 新旧相同 */
    MYSQL_BIND p[2];
    bind_str(&p[0], new_hash);
    bind_str(&p[1], username);
    int n = db_exec("UPDATE user SET password=? WHERE username=?", p, 2);
    if (n <= 0) return -4;
    strncpy(u->pwd_hash, new_hash, LEN_HASH - 1);
    return 0;
}

int auth_delete_user(const char *username)
{
    if (strcmp(username, "admin") == 0) return -2;       /* 保护内置管理员 */
    User **pp = &g_table[hash_index(username)];
    while (*pp) {
        if (strcmp((*pp)->username, username) == 0) {
            User *del = *pp;
            *pp = del->next;
            free(del);
            break;
        }
        pp = &(*pp)->next;
    }
    MYSQL_BIND p[1];
    bind_str(&p[0], username);
    return db_exec("DELETE FROM user WHERE username=?", p, 1) > 0 ? 0 : -1;
}

void auth_list_all(void)
{
    printf("\n");
    print_pad("用户名", 14); print_pad("角色", 10);
    print_pad("绑定学号", 18); putchar('\n');
    print_divider('-', 46);
    int count = 0;
    for (int i = 0; i < HASH_SIZE; ++i) {
        for (User *u = g_table[i]; u; u = u->next) {
            printf("%-14s%-10s%-18s\n", u->username,
                   role_to_str(u->role),
                   is_blank(u->student_id) ? "-" : u->student_id);
            ++count;
        }
    }
    printf("共 %d 个用户。\n", count);
}
