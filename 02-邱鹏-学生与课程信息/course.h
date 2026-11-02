#ifndef COURSE_H
#define COURSE_H

/*============================================================
 * course.h  课程信息模块
 * 负责人：邱鹏
 * 课程数量相对固定、需要按下标随机访问，采用【动态顺序表】存储。
 *==========================================================*/

#include "common.h"

#define COURSE_NAME_LEN 100

typedef struct {
    char course_id[LEN_ID];
    char course_name[COURSE_NAME_LEN];
    float credit;
    char semester[24];
} Course;

int  course_init(void);
void course_free(void);

const Course *course_find(const char *course_id);   /* 找不到返回NULL */
int  course_add(const Course *c);
int  course_update(const Course *c);
int  course_delete(const char *course_id);
void course_list_all(void);
int  course_count(void);

#endif /* COURSE_H */
