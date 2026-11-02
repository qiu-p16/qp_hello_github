/*============================================================
 * menu.c  菜单与交互层实现
 * 负责人：王思航
 *==========================================================*/
#include "../include/menu.h"
#include "../include/auth.h"
#include "../include/student.h"
#include "../include/course.h"
#include "../include/score.h"

/* 读取一个字符串并去空白 */
static void ask_str(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    read_line(buf, size);
    str_trim(buf);
}

/* ---------------- 修改密码(通用) ---------------- */
static void change_my_password(const LoginUser *u)
{
    char oldp[LEN_PWD], np1[LEN_PWD], np2[LEN_PWD];
    printf("请输入原密码：");   read_password(oldp, LEN_PWD);
    printf("请输入新密码：");   read_password(np1, LEN_PWD);
    printf("再次输入新密码："); read_password(np2, LEN_PWD);
    if (strcmp(np1, np2) != 0) { printf("两次输入的新密码不一致。\n"); return; }
    if (is_blank(np1))         { printf("新密码不能为空。\n"); return; }
    int rc = auth_change_password(u->username, oldp, np1);
    switch (rc) {
        case 0:  printf("密码修改成功，下次登录生效。\n"); break;
        case -2: printf("原密码错误。\n"); break;
        case -3: printf("新密码不能与原密码相同。\n"); break;
        default: printf("修改失败。\n");
    }
}

/* ---------------- 学生信息管理(管理员) ---------------- */
static void student_manage(void)
{
    for (;;) {
        printf("\n---- 学生信息管理 ----\n");
        printf("1.新增学生 2.修改学生 3.删除学生 4.全部学生 5.按班级查询 0.返回\n");
        int op = read_int("请选择：", 0, 5);
        if (op == 0) return;
        Student s;
        if (op == 1) {
            memset(&s, 0, sizeof(s));
            ask_str("学号：", s.student_id, LEN_ID);
            ask_str("姓名：", s.name, LEN_NAME);
            ask_str("班级：", s.class_name, LEN_CLASS);
            ask_str("性别(男/女)：", s.gender, 4);
            s.age = read_int("年龄：", 10, 60);
            int rc = student_add(&s);
            printf(rc == 0 ? "新增成功。\n" :
                   rc == -1 ? "该学号已存在。\n" : "新增失败。\n");
        } else if (op == 2) {
            char sid[LEN_ID]; ask_str("要修改的学号：", sid, LEN_ID);
            const Student *old = student_find(sid);
            if (!old) { printf("查无此学生。\n"); continue; }
            s = *old;
            printf("直接回车保留原值：原姓名[%s]\n", old->name);
            char tmp[LEN_LINE];
            ask_str("新姓名：", tmp, LEN_LINE); if (!is_blank(tmp)) strncpy(s.name, tmp, LEN_NAME-1);
            ask_str("新班级：", tmp, LEN_LINE); if (!is_blank(tmp)) strncpy(s.class_name, tmp, LEN_CLASS-1);
            ask_str("新性别：", tmp, LEN_LINE); if (!is_blank(tmp)) strncpy(s.gender, tmp, 3);
            student_update(&s);
            printf("修改完成。\n");
        } else if (op == 3) {
            char sid[LEN_ID]; ask_str("要删除的学号：", sid, LEN_ID);
            student_delete(sid);
        } else if (op == 4) {
            student_list_all();
        } else if (op == 5) {
            char cls[LEN_CLASS]; ask_str("班级名称：", cls, LEN_CLASS);
            student_list_by_class(cls);
        }
    }
}

/* ---------------- 课程信息管理(管理员) ---------------- */
static void course_manage(void)
{
    for (;;) {
        printf("\n---- 课程信息管理 ----\n");
        printf("1.新增课程 2.修改课程 3.删除课程 4.全部课程 0.返回\n");
        int op = read_int("请选择：", 0, 4);
        if (op == 0) return;
        Course c;
        if (op == 1) {
            memset(&c, 0, sizeof(c));
            ask_str("课程号：", c.course_id, LEN_ID);
            ask_str("课程名：", c.course_name, COURSE_NAME_LEN);
            c.credit = (float)read_double("学分：", 0.5, 10);
            ask_str("学期(如2025-1)：", c.semester, 24);
            int rc = course_add(&c);
            printf(rc == 0 ? "新增成功。\n" : rc == -1 ? "课程号已存在。\n" : "新增失败。\n");
        } else if (op == 2) {
            char cid[LEN_ID]; ask_str("要修改的课程号：", cid, LEN_ID);
            const Course *old = course_find(cid);
            if (!old) { printf("查无此课程。\n"); continue; }
            c = *old;
            char tmp[LEN_LINE];
            ask_str("新课程名：", tmp, LEN_LINE); if (!is_blank(tmp)) strncpy(c.course_name, tmp, COURSE_NAME_LEN-1);
            printf("原学分%.1f，输入-1保留：", old->credit);
            double cr = read_double("", -1, 10); if (cr >= 0) c.credit = (float)cr;
            ask_str("新学期：", tmp, LEN_LINE); if (!is_blank(tmp)) strncpy(c.semester, tmp, 23);
            course_update(&c);
            printf("修改完成。\n");
        } else if (op == 3) {
            char cid[LEN_ID]; ask_str("要删除的课程号：", cid, LEN_ID);
            course_delete(cid);
        } else if (op == 4) {
            course_list_all();
        }
    }
}

/* ---------------- 多条件成绩查询 ---------------- */
static void query_scores(void)
{
    char sid[LEN_ID]="", cid[LEN_ID]="", cls[LEN_CLASS]="", sem[24]="";
    printf("以下条件直接回车表示不限制：\n");
    ask_str("学号：", sid, LEN_ID);
    ask_str("课程号：", cid, LEN_ID);
    ask_str("班级：", cls, LEN_CLASS);
    ask_str("学期：", sem, 24);
    int by = read_int("排序字段 1.按成绩 2.按学号：", 1, 2);
    int dir = read_int("顺序 1.降序 2.升序：", 1, 2);
    ScoreSet *rs = score_query(sid, cid, cls, sem);
    score_show_paged(rs, by == 1, dir == 1);
    score_set_free(rs);
}

/* 录入/修改/删除成绩的交互 */
static void input_score(int is_add)
{
    char sid[LEN_ID], cid[LEN_ID], sem[24]="";
    ask_str("学号：", sid, LEN_ID);
    ask_str("课程号：", cid, LEN_ID);
    double v = read_double("成绩(0~100)：", 0, 100);
    if (is_add) { ask_str("学期(可回车)：", sem, 24); score_add(sid, cid, v, sem); }
    else        { score_update(sid, cid, v); }
}

/* ---------------- 用户管理(管理员) ---------------- */
static void user_manage(void)
{
    for (;;) {
        printf("\n---- 用户管理 ----\n");
        printf("1.用户列表 2.新增用户 3.删除用户 0.返回\n");
        int op = read_int("请选择：", 0, 3);
        if (op == 0) return;
        if (op == 1) {
            auth_list_all();
        } else if (op == 2) {
            char name[LEN_USER], pwd[LEN_PWD], sid[LEN_ID]="";
            ask_str("用户名：", name, LEN_USER);
            printf("密码："); read_password(pwd, LEN_PWD);
            int r = read_int("角色 1.管理员 2.教师 3.学生：", 1, 3);
            Role role = (Role)r;
            if (role == ROLE_STUDENT) ask_str("绑定学号：", sid, LEN_ID);
            int rc = auth_add_user(name, pwd, role, sid);
            printf(rc == 0 ? "新增用户成功。\n" : "用户名已存在或新增失败。\n");
        } else if (op == 3) {
            char name[LEN_USER]; ask_str("要删除的用户名：", name, LEN_USER);
            int rc = auth_delete_user(name);
            if (rc == -2) printf("内置管理员 admin 不可删除。\n");
            else printf(rc == 0 ? "已删除。\n" : "删除失败。\n");
        }
    }
}

/* ---------------- 三套角色菜单 ---------------- */
static void admin_menu(const LoginUser *u)
{
    for (;;) {
        printf("\n======== 管理员菜单（%s） ========\n", u->username);
        printf("1.学生信息管理  2.课程信息管理  3.成绩录入\n");
        printf("4.成绩修改      5.成绩删除        6.成绩查询\n");
        printf("7.成绩统计分析  8.成绩排名        9.用户管理\n");
        printf("10.修改密码     0.退出登录\n");
        int op = read_int("请选择：", 0, 10);
        switch (op) {
            case 1: student_manage(); break;
            case 2: course_manage();  break;
            case 3: input_score(1);   break;
            case 4: input_score(0);   break;
            case 5: { char s[LEN_ID],c[LEN_ID];
                      ask_str("学号：",s,LEN_ID); ask_str("课程号：",c,LEN_ID);
                      score_delete(s,c); break; }
            case 6: query_scores();   break;
            case 7: { char c[LEN_ID]="",cl[LEN_CLASS]="",se[24]="";
                      ask_str("课程号(可空)：",c,LEN_ID);
                      ask_str("班级(可空)：",cl,LEN_CLASS);
                      ask_str("学期(可空)：",se,24);
                      score_stat(c,cl,se); break; }
            case 8: { char c[LEN_ID]="",se[24]="";
                      ask_str("课程号(可空则统计全部)：",c,LEN_ID);
                      ask_str("学期(可空)：",se,24);
                      score_rank(c,se); break; }
            case 9: user_manage();    break;
            case 10: change_my_password(u); break;
            case 0: return;
        }
    }
}

static void teacher_menu(const LoginUser *u)
{
    for (;;) {
        printf("\n======== 教师菜单（%s） ========\n", u->username);
        printf("1.录入课程成绩 2.修改课程成绩 3.查看成绩\n");
        printf("4.成绩统计     5.成绩排名     6.修改密码  0.退出登录\n");
        int op = read_int("请选择：", 0, 6);
        switch (op) {
            case 1: input_score(1); break;
            case 2: input_score(0); break;
            case 3: query_scores(); break;
            case 4: { char c[LEN_ID]=""; ask_str("课程号(可空)：",c,LEN_ID);
                      score_stat(c,"",""); break; }
            case 5: { char c[LEN_ID]=""; ask_str("课程号(可空)：",c,LEN_ID);
                      score_rank(c,""); break; }
            case 6: change_my_password(u); break;
            case 0: return;
        }
    }
}

static void student_menu(const LoginUser *u)
{
    for (;;) {
        printf("\n======== 学生菜单（%s 学号%s） ========\n",
               u->username, u->student_id);
        printf("1.查看我的成绩 2.成绩排名查询 3.修改个人信息 4.修改密码 0.退出登录\n");
        int op = read_int("请选择：", 0, 4);
        if (op == 0) return;
        if (op == 1) {
            ScoreSet *rs = score_query_by_student(u->student_id);
            score_show_paged(rs, 1, 1);
            score_set_free(rs);
        } else if (op == 2) {
            char c[LEN_ID]=""; ask_str("课程号(可空则全部)：", c, LEN_ID);
            score_rank(c, "");
        } else if (op == 3) {
            const Student *me = student_find(u->student_id);
            if (!me) { printf("未找到你的学籍信息，请联系管理员。\n"); continue; }
            Student s = *me;
            char tmp[LEN_LINE];
            ask_str("新姓名(回车保留)：", tmp, LEN_LINE); if (!is_blank(tmp)) strncpy(s.name,tmp,LEN_NAME-1);
            ask_str("新班级(回车保留)：", tmp, LEN_LINE); if (!is_blank(tmp)) strncpy(s.class_name,tmp,LEN_CLASS-1);
            ask_str("新性别(回车保留)：", tmp, LEN_LINE); if (!is_blank(tmp)) strncpy(s.gender,tmp,3);
            student_update(&s);
            printf("个人信息已更新。\n");
        } else if (op == 4) {
            change_my_password(u);
        }
    }
}

/* ---------------- 登录 / 注册 ---------------- */
static void do_login(void)
{
    char name[LEN_USER], pwd[LEN_PWD];
    ask_str("用户名：", name, LEN_USER);
    printf("密码："); read_password(pwd, LEN_PWD);
    LoginUser u; memset(&u, 0, sizeof(u));
    int rc = auth_login(name, pwd, &u);
    if (rc != 0) { printf("用户名或密码错误。\n"); return; }
    printf("登录成功，欢迎您，%s（%s）！\n", u.username, role_to_str(u.role));
    switch (u.role) {
        case ROLE_ADMIN:   admin_menu(&u);   break;
        case ROLE_TEACHER: teacher_menu(&u); break;
        case ROLE_STUDENT: student_menu(&u); break;
        default: break;
    }
}

static void do_register(void)
{
    char name[LEN_USER], p1[LEN_PWD], p2[LEN_PWD], sid[LEN_ID];
    ask_str("设置用户名：", name, LEN_USER);
    if (is_blank(name)) { printf("用户名不能为空。\n"); return; }
    printf("设置密码：");   read_password(p1, LEN_PWD);
    printf("确认密码：");   read_password(p2, LEN_PWD);
    if (strcmp(p1, p2) != 0) { printf("两次密码不一致。\n"); return; }
    ask_str("绑定学号：", sid, LEN_ID);
    if (!student_find(sid)) {
        printf("该学号不存在，请先由管理员录入学生信息。\n"); return;
    }
    int rc = auth_register(name, p1, ROLE_STUDENT, sid);
    printf(rc == 0 ? "注册成功，请登录。\n" : "用户名已被占用。\n");
}

/* ---------------- 系统主循环 ---------------- */
void menu_main_loop(void)
{
    for (;;) {
        printf("\n========== 学生成绩管理系统 ==========\n");
        printf("1. 用户登录\n2. 用户注册(学生)\n0. 退出系统\n");
        int op = read_int("请选择操作：", 0, 2);
        if (op == 0) { printf("感谢使用，再见！\n"); return; }
        if (op == 1) do_login();
        else if (op == 2) do_register();
    }
}
