#ifndef AUTH_H
#define AUTH_H

/*============================================================
 * auth.h  用户与权限模块
 * 负责人：孙汶
 * 用户信息采用【链地址法哈希表】按用户名组织，平均 O(1) 精确查找，
 * 数据持久化在 MySQL 的 user 表，内存哈希表做登录缓存。
 *==========================================================*/

#include "common.h"

#define HASH_SIZE 127          /* 哈希桶数量(取素数减少冲突) */

typedef struct User {
    char  username[LEN_USER];
    char  pwd_hash[LEN_HASH];
    Role  role;
    char  student_id[LEN_ID];
    struct User *next;        /* 哈希桶内链指针 */
} User;

/* 启动时从数据库加载全部用户到哈希表，0成功 */
int  auth_init(void);
void auth_free(void);

User *auth_find(const char *username);

/* 登录：校验用户名+密码，成功把用户信息写入 out 并返回0 */
int  auth_login(const char *username, const char *pwd, LoginUser *out);
/* 注册学生账号；用户名重复返回-1 */
int  auth_register(const char *username, const char *pwd,
                   Role role, const char *student_id);
/* 修改密码：校验原密码、新密码不能与原密码相同 */
int  auth_change_password(const char *username, const char *old_pwd,
                          const char *new_pwd);

/* 管理员：用户增删、列表 */
int  auth_add_user(const char *username, const char *pwd,
                   Role role, const char *student_id);
int  auth_delete_user(const char *username);
void auth_list_all(void);

/* 角色 <-> 字符串 */
const char *role_to_str(Role r);
Role        role_from_str(const char *s);

#endif /* AUTH_H */
