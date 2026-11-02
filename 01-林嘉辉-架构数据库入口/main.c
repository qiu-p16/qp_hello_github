/*============================================================
 * main.c  学生成绩管理系统 程序入口
 * 负责人：林嘉辉（组长）
 * 职责：连接数据库 → 加载内存数据 → 进入主菜单 → 释放资源
 * 编译：见 build.bat（MinGW + MySQL Connector/C）
 *==========================================================*/
#include "../include/common.h"
#include "../include/db.h"
#include "../include/auth.h"
#include "../include/student.h"
#include "../include/course.h"
#include "../include/menu.h"

int main(void)
{
    console_init();
    print_divider('=', 40);
    printf("       学生成绩管理系统  C99 + MySQL\n");
    print_divider('=', 40);

    /* 1. 连接数据库 */
    if (db_connect() != 0) {
        printf("\n无法连接数据库，程序退出。\n");
        printf("排查步骤：\n");
        printf("  1) 启动 MySQL 服务；\n");
        printf("  2) 在 DataGrip 中执行 sql/schema.sql 建库；\n");
        printf("  3) 修改 include/db.h 中的 DB_USER/DB_PASS。\n");
        pause_hint();
        return 1;
    }
    printf("数据库连接成功。\n");

    /* 2. 加载数据到内存数据结构（哈希表/BST/顺序表） */
    if (auth_init() != 0 || student_init() != 0 || course_init() != 0) {
        printf("数据加载失败，程序退出。\n");
        db_disconnect();
        return 1;
    }
    printf("已加载 用户/学生/课程 数据。\n");
    printf("内置账号：管理员 admin/123456，教师 teacher/123456，学生 student1/123456\n");

    /* 3. 进入主菜单 */
    menu_main_loop();

    /* 4. 释放资源 */
    student_free();
    course_free();
    auth_free();
    db_disconnect();
    return 0;
}
