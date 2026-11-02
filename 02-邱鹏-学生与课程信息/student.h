#ifndef STUDENT_H
#define STUDENT_H

/*============================================================
 * student.h  学生信息模块
 * 负责人：邱鹏
 * 学生以【学号为关键字的二叉搜索树(BST)】组织，
 * 支持 O(log n) 的插入/查找/删除，中序遍历即按学号有序输出。
 *==========================================================*/

#include "common.h"

typedef struct {
    char student_id[LEN_ID];
    char name[LEN_NAME];
    char class_name[LEN_CLASS];
    char gender[4];
    int  age;
} Student;

typedef struct StuNode {
    Student data;
    struct StuNode *left, *right;
} StuNode;

int  student_init(void);              /* 从数据库加载到 BST */
void student_free(void);

const Student *student_find(const char *student_id);

int  student_add(const Student *s);   /* 新增：写库并建树，重复返回-1 */
int  student_update(const Student *s);
int  student_delete(const char *student_id);

void student_list_all(void);          /* 中序遍历全部 */
void student_list_by_class(const char *class_name);
int  student_count(void);

#endif /* STUDENT_H */
